// Rescue Keyboard: a BLE keyboard + mouse paired to the Mac, driven from a
// PIN-protected web page on the ESP32's own Wi-Fi network.
//
// Threads: web/WebSocket callbacks run in the AsyncTCP task and only parse and
// enqueue. All BLE HID work happens in loop(), so HID state has one owner.

#include <WiFi.h>
#include <DNSServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <esp_random.h>

#include "config.h"
#include "ble_hid.h"
#include "web_ui.h"

static const size_t MAX_TEXT_LEN = 256;
static const uint32_t MOUSE_REPORT_INTERVAL_MS = 12;
// Let go of held keys/buttons if the controlling page goes quiet this long
// (phone locked mid-drag, laptop lost a keyup, etc.).
static const uint32_t HELD_INPUT_TIMEOUT_MS = 15000;
static const uint8_t MAX_WS_CLIENTS = 2;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dns;
BleHid hid;

// ---- Command queue (AsyncTCP task -> loop) -------------------------------------

enum CmdType : uint8_t {
  CMD_KEY_DOWN,     // a = HID code
  CMD_KEY_UP,       // a = HID code
  CMD_TAP,          // a = HID code, b = modifier bits
  CMD_CHAR,         // a = ASCII char, b = extra modifier bits
  CMD_BTN_DOWN,     // a = button bits
  CMD_BTN_UP,       // a = button bits
  CMD_CLICK,        // a = button bits
  CMD_CONSUMER,     // b = usage
  CMD_RELEASE_ALL,
};

struct HidCmd {
  uint8_t type;
  uint8_t a;
  uint16_t b;
};

static QueueHandle_t cmdQueue;

// Mouse movement isn't queued: it's summed here and sent at a steady rate,
// which keeps the pointer smooth no matter how bursty the Wi-Fi side is.
static portMUX_TYPE mouseLock = portMUX_INITIALIZER_UNLOCKED;
static int32_t pendingDx = 0, pendingDy = 0, pendingWheel = 0, pendingPan = 0;

static volatile uint32_t lastInputMs = 0;

static bool enqueue(uint8_t type, uint8_t a = 0, uint16_t b = 0) {
  HidCmd cmd = {type, a, b};
  return xQueueSend(cmdQueue, &cmd, 0) == pdTRUE;
}

// ---- Auth state (only touched from the AsyncTCP task) ----------------------------

static String sessionToken;  // empty = nobody logged in
static volatile uint32_t sessionLastUsedMs = 0;
static volatile bool sessionActive = false;  // mirror of !sessionToken.isEmpty() for loop()
static uint8_t failedAttempts = 0;
static uint8_t lockoutCount = 0;
static uint32_t lockedUntilMs = 0;

static bool constantTimeEquals(const String &a, const char *b) {
  size_t lenB = strlen(b);
  uint8_t diff = a.length() != lenB;
  for (size_t i = 0; i < a.length(); i++) {
    diff |= (uint8_t)a[i] ^ (uint8_t)b[i % (lenB ? lenB : 1)];
  }
  return diff == 0;
}

static String randomToken() {
  char buf[33];
  for (int i = 0; i < 4; i++) snprintf(buf + i * 8, 9, "%08lx", (unsigned long)esp_random());
  return String(buf);
}

static uint32_t lockoutRemainingSec() {
  int32_t remaining = (int32_t)(lockedUntilMs - millis());
  return remaining > 0 ? (remaining + 999) / 1000 : 0;
}

static String cookieValue(const String &cookies, const char *name) {
  String key = String(name) + "=";
  int start = cookies.indexOf(key);
  if (start < 0) return "";
  start += key.length();
  int end = cookies.indexOf(';', start);
  return cookies.substring(start, end < 0 ? cookies.length() : end);
}

static bool sessionValid() {
  if (sessionToken.isEmpty()) return false;
  if (millis() - sessionLastUsedMs > SESSION_IDLE_SECONDS * 1000UL) {
    sessionToken = "";
    sessionActive = false;
    return false;
  }
  return true;
}

static bool isAuthed(AsyncWebServerRequest *req) {
  if (!sessionValid()) return false;
  String token = cookieValue(req->header("Cookie"), "rk_session");
  if (token.length() != sessionToken.length() || !constantTimeEquals(token, sessionToken.c_str())) {
    return false;
  }
  sessionLastUsedMs = millis();
  return true;
}

// Every POST must carry X-Rescue. Browsers won't add a custom header to a
// cross-site request without a CORS preflight, which we never allow.
static bool checkPost(AsyncWebServerRequest *req) {
  if (req->header("X-Rescue") != "1") {
    req->send(400, "text/plain", "Bad request.");
    return false;
  }
  return true;
}

static bool sameOrigin(AsyncWebServerRequest *req) {
  const String &origin = req->header("Origin");
  return origin.isEmpty() || origin == "http://" + WiFi.softAPIP().toString();
}

// ---- HTTP handlers ---------------------------------------------------------------

static void handleIndex(AsyncWebServerRequest *req) {
  Serial.println("HTTP: page served");
  AsyncWebServerResponse *res =
      req->beginResponse(200, "text/html", (const uint8_t *)INDEX_HTML, strlen(INDEX_HTML));
  res->addHeader("Cache-Control", "no-store");
  req->send(res);
}

static void handleStatus(AsyncWebServerRequest *req) {
  String json = String("{\"authed\":") + (isAuthed(req) ? "true" : "false") +
                ",\"ble\":" + (hid.isConnected() ? "true" : "false") + "}";
  AsyncWebServerResponse *res = req->beginResponse(200, "application/json", json);
  res->addHeader("Cache-Control", "no-store");
  req->send(res);
}

static void handleLogin(AsyncWebServerRequest *req) {
  if (!checkPost(req)) return;

  uint32_t wait = lockoutRemainingSec();
  if (wait > 0) {
    req->send(429, "text/plain", "Too many wrong PINs. Try again in " + String(wait) + " s.");
    return;
  }

  const AsyncWebParameter *pin = req->getParam("pin", true);
  if (!pin || !constantTimeEquals(pin->value(), WEB_PIN)) {
    failedAttempts++;
    Serial.printf("Wrong PIN (%u/%u)\n", failedAttempts, MAX_FAILED_ATTEMPTS);
    if (failedAttempts >= MAX_FAILED_ATTEMPTS) {
      uint32_t secs = LOCKOUT_BASE_SECONDS << min<uint8_t>(lockoutCount, 6);
      if (secs > LOCKOUT_MAX_SECONDS) secs = LOCKOUT_MAX_SECONDS;
      lockedUntilMs = millis() + secs * 1000UL;
      lockoutCount++;
      failedAttempts = 0;
      req->send(429, "text/plain", "Too many wrong PINs. Locked for " + String(secs) + " s.");
    } else {
      req->send(401, "text/plain", "Wrong PIN.");
    }
    return;
  }

  failedAttempts = 0;
  lockoutCount = 0;
  ws.closeAll();                 // a new login ends any previous session
  sessionToken = randomToken();
  sessionLastUsedMs = millis();
  sessionActive = true;
  AsyncWebServerResponse *res = req->beginResponse(200, "text/plain", "OK");
  res->addHeader("Set-Cookie", "rk_session=" + sessionToken + "; Path=/; HttpOnly; SameSite=Strict");
  req->send(res);
  Serial.println("Logged in");
}

static void handleLogout(AsyncWebServerRequest *req) {
  if (!checkPost(req)) return;
  sessionToken = "";
  sessionActive = false;
  ws.closeAll();
  AsyncWebServerResponse *res = req->beginResponse(200, "text/plain", "OK");
  res->addHeader("Set-Cookie", "rk_session=; Path=/; Max-Age=0");
  req->send(res);
}

// ---- WebSocket protocol ------------------------------------------------------------
//
// One text command per message:
//   m <dx> <dy>      move pointer            w <v> <h>    scroll (wheel, pan)
//   bd|bu|bc <bits>  button down/up/click    ra           release everything
//   kd|ku <code>     key down/up (HID code)  kt <code> <mods>  tap key with modifiers
//   t <text>         type text (\n = Enter)  kc <mods> <char>  type char with modifiers
//   bs <n>           backspace n times       cc <usage>   media key (consumer usage)
// Server -> page: "ble 1" / "ble 0", "err <message>".

static void handleCommand(AsyncWebSocketClient *client, const char *msg, size_t len) {
  if (len < 2) return;
  char op[3] = {msg[0], msg[1] == ' ' ? '\0' : msg[1], '\0'};
  const char *args = msg + (op[1] ? 2 : 1);
  if (*args == ' ') args++;
  long a = 0, b = 0;

  if (!strcmp(op, "m") || !strcmp(op, "w")) {
    if (sscanf(args, "%ld %ld", &a, &b) != 2) return;
    a = constrain(a, -2000L, 2000L);
    b = constrain(b, -2000L, 2000L);
    portENTER_CRITICAL(&mouseLock);
    if (op[0] == 'm') {
      pendingDx += a;
      pendingDy += b;
    } else {
      pendingWheel += a;
      pendingPan += b;
    }
    portEXIT_CRITICAL(&mouseLock);
    return;
  }

  bool ok = true;
  if (!strcmp(op, "t")) {
    size_t n = len - (args - msg);
    if (n > MAX_TEXT_LEN || uxQueueSpacesAvailable(cmdQueue) < n) {
      client->text("err Too much text at once, try again.");
      return;
    }
    for (size_t i = 0; i < n; i++) enqueue(CMD_CHAR, (uint8_t)args[i], 0);
  } else if (!strcmp(op, "kc")) {
    if (sscanf(args, "%ld", &a) != 1) return;
    const char *sp = strchr(args, ' ');
    if (!sp || !sp[1]) return;
    ok = enqueue(CMD_CHAR, (uint8_t)sp[1], (uint16_t)(a & 0xFF));
  } else if (!strcmp(op, "bs")) {
    if (sscanf(args, "%ld", &a) != 1) return;
    a = constrain(a, 0L, 64L);
    if (uxQueueSpacesAvailable(cmdQueue) < (UBaseType_t)a) {
      client->text("err Busy, try again.");
      return;
    }
    for (long i = 0; i < a; i++) enqueue(CMD_TAP, HID_KEY_BACKSPACE, 0);
  } else if (!strcmp(op, "kd") || !strcmp(op, "ku")) {
    if (sscanf(args, "%ld", &a) != 1 || a <= 0 || a > 0xE7) return;
    ok = enqueue(op[1] == 'd' ? CMD_KEY_DOWN : CMD_KEY_UP, (uint8_t)a);
  } else if (!strcmp(op, "kt")) {
    if (sscanf(args, "%ld %ld", &a, &b) < 1 || a < 0 || a > 0xE7) return;
    ok = enqueue(CMD_TAP, (uint8_t)a, (uint16_t)(b & 0xFF));
  } else if (!strcmp(op, "bd") || !strcmp(op, "bu") || !strcmp(op, "bc")) {
    if (sscanf(args, "%ld", &a) != 1) return;
    uint8_t type = op[1] == 'd' ? CMD_BTN_DOWN : op[1] == 'u' ? CMD_BTN_UP : CMD_CLICK;
    ok = enqueue(type, (uint8_t)(a & 0x07));
  } else if (!strcmp(op, "cc")) {
    if (sscanf(args, "%ld", &a) != 1 || a <= 0 || a > 0x3FF) return;
    ok = enqueue(CMD_CONSUMER, 0, (uint16_t)a);
  } else if (!strcmp(op, "ra")) {
    ok = enqueue(CMD_RELEASE_ALL);
  }
  if (!ok) client->text("err Busy, try again.");
}

static void onWsEvent(AsyncWebSocket *, AsyncWebSocketClient *client, AwsEventType type,
                      void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    client->text(hid.isConnected() ? "ble 1" : "ble 0");
    Serial.println("Controller connected");
  } else if (type == WS_EVT_DISCONNECT) {
    enqueue(CMD_RELEASE_ALL);  // never leave a key or button stuck down
    Serial.println("Controller disconnected");
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    // Commands are tiny; ignore anything fragmented or not text.
    if (!info->final || info->index != 0 || info->len != len || info->opcode != WS_TEXT) return;
    if (!sessionValid()) {
      client->close();
      return;
    }
    sessionLastUsedMs = millis();
    lastInputMs = millis();
    char msg[MAX_TEXT_LEN + 8];
    if (len >= sizeof(msg)) {
      client->text("err Too much text at once.");
      return;
    }
    memcpy(msg, data, len);
    msg[len] = '\0';
    handleCommand(client, msg, len);
  }
}

// ---- HID pump (loop task) ----------------------------------------------------------

static int8_t takeClamped(int32_t &pending) {
  int32_t v = constrain(pending, -127, 127);
  pending -= v;
  return (int8_t)v;
}

static void flushMouse() {
  portENTER_CRITICAL(&mouseLock);
  int8_t dx = takeClamped(pendingDx);
  int8_t dy = takeClamped(pendingDy);
  int8_t wheel = takeClamped(pendingWheel);
  int8_t pan = takeClamped(pendingPan);
  portEXIT_CRITICAL(&mouseLock);
  if (dx || dy || wheel || pan) hid.mouseMove(dx, dy, wheel, pan);
}

static void runCommand(const HidCmd &cmd) {
  switch (cmd.type) {
    case CMD_KEY_DOWN: hid.keyDown(cmd.a); break;
    case CMD_KEY_UP: hid.keyUp(cmd.a); break;
    case CMD_TAP: hid.tap(cmd.a, (uint8_t)cmd.b); break;
    case CMD_CHAR: hid.typeChar((char)cmd.a, (uint8_t)cmd.b); break;
    case CMD_BTN_DOWN:
      flushMouse();  // movement that came before the press belongs before it
      hid.setMouseButtons(hid.mouseButtons() | cmd.a);
      break;
    case CMD_BTN_UP:
      flushMouse();
      hid.setMouseButtons(hid.mouseButtons() & ~cmd.a);
      break;
    case CMD_CLICK:
      flushMouse();
      hid.setMouseButtons(hid.mouseButtons() | cmd.a);
      delay(15);
      hid.setMouseButtons(hid.mouseButtons() & ~cmd.a);
      break;
    case CMD_CONSUMER: hid.consumerTap(cmd.b); break;
    case CMD_RELEASE_ALL: hid.releaseAll(); break;
  }
}

// ---- Setup / loop ------------------------------------------------------------------

static void logHeap(const char *when) {
  Serial.printf("Heap (%s): free %u, lowest %u, largest block %u\n", when, ESP.getFreeHeap(),
                ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nRescue Keyboard starting");
  logHeap("boot");

  cmdQueue = xQueueCreate(320, sizeof(HidCmd));
  hid.begin(BLE_DEVICE_NAME);
  logHeap("after BLE");

  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    if (event == ARDUINO_EVENT_WIFI_AP_STACONNECTED) {
      Serial.println("Wi-Fi: phone joined");
    } else if (event == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) {
      Serial.printf("Wi-Fi: phone left (reason %u)\n", info.wifi_ap_stadisconnected.reason);
    } else if (event == ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED) {
      Serial.println("Wi-Fi: phone got an IP");
    }
  });
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(AP_SSID, AP_PASSWORD, 6, 0, 2)) {  // max 2 clients
    Serial.println("Failed to start Wi-Fi AP (password must be 8+ chars)");
  }
  Serial.printf("Wi-Fi \"%s\" up, open http://%s\n", AP_SSID, WiFi.softAPIP().toString().c_str());

  ws.onEvent(onWsEvent);
  ws.handleHandshake([](AsyncWebServerRequest *req) { return sameOrigin(req) && isAuthed(req); });
  server.addHandler(&ws);

  server.on("/", HTTP_GET, handleIndex);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/login", HTTP_POST, handleLogin);
  server.on("/api/logout", HTTP_POST, handleLogout);
  // Captive portal: every DNS name resolves to us, and every unknown URL (including
  // the phone's connectivity checks) redirects to the page, so the OS pops it up on join.
  server.onNotFound([](AsyncWebServerRequest *req) {
    Serial.printf("HTTP: redirecting %s%s\n", req->host().c_str(), req->url().c_str());
    req->redirect("http://" + WiFi.softAPIP().toString() + "/");
  });
  server.begin();
  dns.start(53, "*", WiFi.softAPIP());
  logHeap("ready");
}

void loop() {
  static uint32_t lastMouseMs = 0;
  static uint32_t lastHousekeepingMs = 0;
  static bool lastBle = false;

  dns.processNextRequest();
  hid.poll();

  uint32_t now = millis();
  if (now - lastMouseMs >= MOUSE_REPORT_INTERVAL_MS) {
    lastMouseMs = now;
    flushMouse();
  }

  // One command per pass so mouse reports keep flowing while text is typed.
  HidCmd cmd;
  if (xQueueReceive(cmdQueue, &cmd, 0) == pdTRUE) runCommand(cmd);

  bool ble = hid.isConnected();
  if (ble != lastBle) {
    lastBle = ble;
    ws.textAll(ble ? "ble 1" : "ble 0");
  }

  if (now - lastHousekeepingMs >= 1000) {
    lastHousekeepingMs = now;
    // Log whenever free memory hits a new low, in 4 KB steps.
    static uint32_t loggedLow = UINT32_MAX;
    if (ESP.getMinFreeHeap() + 4096 < loggedLow) {
      loggedLow = ESP.getMinFreeHeap();
      logHeap("new low");
    }
    ws.cleanupClients(MAX_WS_CLIENTS);
    if (hid.anythingHeld() && (ws.count() == 0 || now - lastInputMs > HELD_INPUT_TIMEOUT_MS)) {
      hid.releaseAll();
    }
    if (sessionActive && ws.count() > 0 && now - sessionLastUsedMs > SESSION_IDLE_SECONDS * 1000UL) {
      ws.closeAll();  // the page will notice and show the PIN screen again
    }
  }

  delay(1);
}
