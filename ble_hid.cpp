#include "ble_hid.h"

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

// Delay after each keyboard report. Too short and macOS can drop repeated letters.
static const uint32_t KEY_DELAY_MS = 10;

enum : uint8_t { REPORT_KEYBOARD = 1, REPORT_MOUSE = 2, REPORT_CONSUMER = 3 };

static const uint8_t REPORT_MAP[] = {
  // ---- Keyboard (report 1): modifiers, reserved, 6 keys; LED output ----
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x06,        // Usage (Keyboard)
  0xA1, 0x01,        // Collection (Application)
  0x85, REPORT_KEYBOARD,
  0x05, 0x07,        //   Usage Page (Keyboard)
  0x19, 0xE0,        //   Usage Minimum (Left Control)
  0x29, 0xE7,        //   Usage Maximum (Right GUI)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x08,        //   Report Count (8)
  0x81, 0x02,        //   Input (Data, Variable, Absolute)  ; modifier bits
  0x95, 0x01,        //   Report Count (1)
  0x75, 0x08,        //   Report Size (8)
  0x81, 0x01,        //   Input (Constant)                  ; reserved byte
  0x95, 0x05,        //   Report Count (5)
  0x75, 0x01,        //   Report Size (1)
  0x05, 0x08,        //   Usage Page (LEDs)
  0x19, 0x01,        //   Usage Minimum (Num Lock)
  0x29, 0x05,        //   Usage Maximum (Kana)
  0x91, 0x02,        //   Output (Data, Variable, Absolute)  ; LED report
  0x95, 0x01,        //   Report Count (1)
  0x75, 0x03,        //   Report Size (3)
  0x91, 0x01,        //   Output (Constant)                  ; LED padding
  0x95, 0x06,        //   Report Count (6)
  0x75, 0x08,        //   Report Size (8)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xE7, 0x00,  //   Logical Maximum (231; 2 bytes, 1-byte form is signed)
  0x05, 0x07,        //   Usage Page (Keyboard)
  0x19, 0x00,        //   Usage Minimum (0)
  0x29, 0xE7,        //   Usage Maximum (231)
  0x81, 0x00,        //   Input (Data, Array)                ; up to 6 keys
  0xC0,              // End Collection

  // ---- Mouse (report 2): 5 buttons, X, Y, wheel, horizontal pan ----
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x02,        // Usage (Mouse)
  0xA1, 0x01,        // Collection (Application)
  0x85, REPORT_MOUSE,
  0x09, 0x01,        //   Usage (Pointer)
  0xA1, 0x00,        //   Collection (Physical)
  0x05, 0x09,        //     Usage Page (Button)
  0x19, 0x01,        //     Usage Minimum (1)
  0x29, 0x05,        //     Usage Maximum (5)
  0x15, 0x00,        //     Logical Minimum (0)
  0x25, 0x01,        //     Logical Maximum (1)
  0x95, 0x05,        //     Report Count (5)
  0x75, 0x01,        //     Report Size (1)
  0x81, 0x02,        //     Input (Data, Variable, Absolute)
  0x95, 0x01,        //     Report Count (1)
  0x75, 0x03,        //     Report Size (3)
  0x81, 0x01,        //     Input (Constant)                ; padding
  0x05, 0x01,        //     Usage Page (Generic Desktop)
  0x09, 0x30,        //     Usage (X)
  0x09, 0x31,        //     Usage (Y)
  0x09, 0x38,        //     Usage (Wheel)
  0x15, 0x81,        //     Logical Minimum (-127)
  0x25, 0x7F,        //     Logical Maximum (127)
  0x75, 0x08,        //     Report Size (8)
  0x95, 0x03,        //     Report Count (3)
  0x81, 0x06,        //     Input (Data, Variable, Relative)
  0x05, 0x0C,        //     Usage Page (Consumer)
  0x0A, 0x38, 0x02,  //     Usage (AC Pan)
  0x15, 0x81,        //     Logical Minimum (-127)
  0x25, 0x7F,        //     Logical Maximum (127)
  0x75, 0x08,        //     Report Size (8)
  0x95, 0x01,        //     Report Count (1)
  0x81, 0x06,        //     Input (Data, Variable, Relative)
  0xC0,              //   End Collection
  0xC0,              // End Collection

  // ---- Consumer control (report 3): one 16-bit usage ----
  0x05, 0x0C,        // Usage Page (Consumer)
  0x09, 0x01,        // Usage (Consumer Control)
  0xA1, 0x01,        // Collection (Application)
  0x85, REPORT_CONSUMER,
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x03,  //   Logical Maximum (1023)
  0x19, 0x00,        //   Usage Minimum (0)
  0x2A, 0xFF, 0x03,  //   Usage Maximum (1023)
  0x75, 0x10,        //   Report Size (16)
  0x95, 0x01,        //   Report Count (1)
  0x81, 0x00,        //   Input (Data, Array)
  0xC0,              // End Collection
};

static volatile bool connected = false;
static volatile bool needsAdvertising = false;
static volatile uint32_t disconnectedAtMs = 0;

// The ESP32 has one radio for BLE and Wi-Fi. Once connected, ask the Mac for a
// 15-30 ms interval with slave latency 4, so we can skip connection events when
// idle and leave Wi-Fi more airtime. Values follow Apple's accessory guidelines.
static const uint32_t CONN_PARAMS_DELAY_MS = 3000;
static const uint16_t CONN_INTERVAL_MIN = 12;  // x 1.25 ms = 15 ms
static const uint16_t CONN_INTERVAL_MAX = 24;  // x 1.25 ms = 30 ms
static const uint16_t CONN_LATENCY = 4;
static const uint16_t CONN_TIMEOUT = 400;      // x 10 ms = 4 s

static NimBLEServer *bleServer = nullptr;
static volatile uint16_t connHandle = 0;
static volatile bool connParamsPending = false;
static volatile uint32_t connectedAtMs = 0;

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *, NimBLEConnInfo &info) override {
    connected = true;
    connHandle = info.getConnHandle();
    connectedAtMs = millis();
    connParamsPending = true;
    Serial.println("BLE: Mac connected");
  }
  void onDisconnect(NimBLEServer *, NimBLEConnInfo &, int reason) override {
    connected = false;
    connParamsPending = false;
    needsAdvertising = true;
    disconnectedAtMs = millis();
    Serial.printf("BLE: disconnected (reason 0x%x)\n", reason);
  }
  void onAuthenticationComplete(NimBLEConnInfo &info) override {
    Serial.println(info.isEncrypted() ? "BLE: paired/encrypted" : "BLE: pairing failed");
  }
  void onConnParamsUpdate(NimBLEConnInfo &info) override {
    Serial.printf("BLE: interval %.2f ms, latency %u, timeout %u ms\n", info.getConnInterval() * 1.25f,
                  info.getConnLatency(), info.getConnTimeout() * 10);
  }
};

void BleHid::begin(const char *deviceName) {
  // NimBLE instead of the core's Bluedroid-based BLE library: it uses ~50 KB less
  // RAM, which Wi-Fi and the web server need (Bluedroid left ~20 KB free and the
  // board crashed out of memory when a phone opened several connections).
  NimBLEDevice::init(deviceName);

  // "Just Works" bonding: no passkey (we have no screen), keys stored for reconnects.
  NimBLEDevice::setSecurityAuth(true, false, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

  bleServer = NimBLEDevice::createServer();
  bleServer->setCallbacks(new ServerCallbacks());
  bleServer->advertiseOnDisconnect(false);  // poll() restarts it after a short pause

  NimBLEHIDDevice *hid = new NimBLEHIDDevice(bleServer);
  hid->setManufacturer("DIY");
  hid->setPnp(0x02, 0x303A, 0x4B52, 0x0300);  // USB-IF source, Espressif VID, arbitrary PID
  hid->setHidInfo(0x00, 0x01);                // no country code, remote wake
  hid->setReportMap((uint8_t *)REPORT_MAP, sizeof(REPORT_MAP));
  keyboardInput = hid->getInputReport(REPORT_KEYBOARD);
  hid->getOutputReport(REPORT_KEYBOARD);      // LED report; macOS expects it on keyboards
  mouseInput = hid->getInputReport(REPORT_MOUSE);
  consumerInput = hid->getInputReport(REPORT_CONSUMER);
  hid->setBatteryLevel(100);
  bleServer->start();

  NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
  adv->setName(deviceName);
  adv->setAppearance(0x03C1);  // HID keyboard
  adv->addServiceUUID(hid->getHidService()->getUUID());
  adv->enableScanResponse(true);
  NimBLEDevice::startAdvertising();
  Serial.println("BLE: advertising");
}

bool BleHid::isConnected() const {
  return connected;
}

void BleHid::poll() {
  if (connParamsPending && millis() - connectedAtMs > CONN_PARAMS_DELAY_MS) {
    connParamsPending = false;
    bleServer->updateConnParams(connHandle, CONN_INTERVAL_MIN, CONN_INTERVAL_MAX, CONN_LATENCY, CONN_TIMEOUT);
    Serial.println("BLE: requested a longer connection interval");
  }
  if (needsAdvertising && millis() - disconnectedAtMs > 500) {
    needsAdvertising = false;
    NimBLEDevice::startAdvertising();
    Serial.println("BLE: advertising again");
  }
}

// ---- Keyboard ------------------------------------------------------------------

void BleHid::sendKeyboard(uint8_t extraModifiers, uint8_t extraCode) {
  if (!connected) return;
  uint8_t report[8] = {(uint8_t)(heldModifiers | extraModifiers), 0};
  int slot = 2;
  for (uint8_t k : heldKeys) {
    if (k) report[slot++] = k;
  }
  if (extraCode && slot < 8) report[slot] = extraCode;
  keyboardInput->notify(report, sizeof(report));
  delay(KEY_DELAY_MS);
}

void BleHid::keyDown(uint8_t code) {
  if (code >= 0xE0 && code <= 0xE7) {
    heldModifiers |= 1 << (code - 0xE0);
  } else {
    for (uint8_t k : heldKeys) {
      if (k == code) return;
    }
    for (uint8_t &k : heldKeys) {
      if (k == 0) {
        k = code;
        break;
      }
    }
  }
  sendKeyboard(0, 0);
}

void BleHid::keyUp(uint8_t code) {
  if (code >= 0xE0 && code <= 0xE7) {
    heldModifiers &= ~(1 << (code - 0xE0));
  } else {
    for (uint8_t &k : heldKeys) {
      if (k == code) k = 0;
    }
  }
  sendKeyboard(0, 0);
}

void BleHid::tap(uint8_t code, uint8_t modifiers) {
  // Press modifiers on their own first: some apps ignore a chord whose
  // modifier arrives in the same report as the key.
  if (modifiers) sendKeyboard(modifiers, 0);
  sendKeyboard(modifiers, code);
  sendKeyboard(modifiers, 0);
  if (modifiers) sendKeyboard(0, 0);
}

bool BleHid::typeChar(char c, uint8_t extraModifiers) {
  KeyStroke k = asciiToKey(c);
  if (k.code == 0) return false;
  if (extraModifiers) {
    tap(k.code, k.modifiers | extraModifiers);
  } else {
    // Plain typing: one down/up pair per character is enough and twice as fast.
    sendKeyboard(k.modifiers, k.code);
    sendKeyboard(0, 0);
  }
  return true;
}

// ---- Mouse ---------------------------------------------------------------------

void BleHid::sendMouse(int8_t dx, int8_t dy, int8_t wheel, int8_t pan) {
  if (!connected) return;
  uint8_t report[5] = {buttons, (uint8_t)dx, (uint8_t)dy, (uint8_t)wheel, (uint8_t)pan};
  mouseInput->notify(report, sizeof(report));
}

void BleHid::setMouseButtons(uint8_t b) {
  if (b == buttons) return;
  buttons = b;
  sendMouse(0, 0, 0, 0);
}

void BleHid::mouseMove(int8_t dx, int8_t dy, int8_t wheel, int8_t pan) {
  sendMouse(dx, dy, wheel, pan);
}

// ---- Consumer ------------------------------------------------------------------

void BleHid::consumerTap(uint16_t usage) {
  if (!connected) return;
  uint8_t down[2] = {(uint8_t)usage, (uint8_t)(usage >> 8)};
  uint8_t up[2] = {0, 0};
  consumerInput->notify(down, sizeof(down));
  delay(KEY_DELAY_MS);
  consumerInput->notify(up, sizeof(up));
  delay(KEY_DELAY_MS);
}

// ---- Shared --------------------------------------------------------------------

void BleHid::releaseAll() {
  bool hadKeys = heldModifiers != 0;
  for (uint8_t &k : heldKeys) {
    if (k) hadKeys = true;
    k = 0;
  }
  heldModifiers = 0;
  if (hadKeys) sendKeyboard(0, 0);
  setMouseButtons(0);
}

bool BleHid::anythingHeld() const {
  if (heldModifiers || buttons) return true;
  for (uint8_t k : heldKeys) {
    if (k) return true;
  }
  return false;
}
