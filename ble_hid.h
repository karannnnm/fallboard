#pragma once
#include <Arduino.h>
#include "keymap.h"

class NimBLECharacteristic;

#define MOUSE_LEFT   0x01
#define MOUSE_RIGHT  0x02
#define MOUSE_MIDDLE 0x04

// BLE HID (HOGP) composite device: keyboard + mouse + consumer (media) keys,
// built on NimBLE-Arduino. "Just Works" bonding; bond keys persist
// in NVS across power cycles and normal re-flashes.
//
// Not thread-safe: call everything from one task (loop()).
class BleHid {
public:
  void begin(const char *deviceName);
  bool isConnected() const;
  // Restarts advertising after a disconnect.
  void poll();

  // Keyboard. Codes 0xE0-0xE7 are modifiers (Left Ctrl ... Right GUI).
  void keyDown(uint8_t code);
  void keyUp(uint8_t code);
  // Press and release one key with extra modifiers, on top of anything held.
  void tap(uint8_t code, uint8_t modifiers = 0);
  // Types one ASCII character. Returns false if it has no mapping.
  bool typeChar(char c, uint8_t extraModifiers = 0);

  // Mouse. Deltas are clamped to one report's range; callers carry the remainder.
  void setMouseButtons(uint8_t buttons);
  uint8_t mouseButtons() const { return buttons; }
  void mouseMove(int8_t dx, int8_t dy, int8_t wheel, int8_t pan);

  // Consumer control usage, e.g. 0xE9 volume up.
  void consumerTap(uint16_t usage);

  // Lets go of every key and mouse button.
  void releaseAll();
  bool anythingHeld() const;

private:
  void sendKeyboard(uint8_t extraModifiers, uint8_t extraCode);
  void sendMouse(int8_t dx, int8_t dy, int8_t wheel, int8_t pan);

  NimBLECharacteristic *keyboardInput = nullptr;
  NimBLECharacteristic *mouseInput = nullptr;
  NimBLECharacteristic *consumerInput = nullptr;
  uint8_t heldModifiers = 0;
  uint8_t heldKeys[6] = {0};
  uint8_t buttons = 0;
};
