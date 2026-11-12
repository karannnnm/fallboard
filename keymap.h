// ASCII -> HID usage codes, assuming the Mac uses a US (ANSI) keyboard layout.
#pragma once
#include <stdint.h>

#define KEY_MOD_LSHIFT 0x02

#define HID_KEY_ENTER     0x28
#define HID_KEY_ESC       0x29
#define HID_KEY_BACKSPACE 0x2A
#define HID_KEY_TAB       0x2B
#define HID_KEY_SPACE     0x2C
#define HID_KEY_RIGHT     0x4F
#define HID_KEY_LEFT      0x50
#define HID_KEY_DOWN      0x51
#define HID_KEY_UP        0x52

struct KeyStroke {
  uint8_t modifiers;
  uint8_t code;  // 0 = no mapping for this character
};

struct PunctKey {
  char c;
  uint8_t code;
  bool shift;
};

static const PunctKey PUNCT_KEYS[] = {
  {' ', 0x2C, false}, {'!', 0x1E, true},  {'"', 0x34, true},  {'#', 0x20, true},
  {'$', 0x21, true},  {'%', 0x22, true},  {'&', 0x24, true},  {'\'', 0x34, false},
  {'(', 0x26, true},  {')', 0x27, true},  {'*', 0x25, true},  {'+', 0x2E, true},
  {',', 0x36, false}, {'-', 0x2D, false}, {'.', 0x37, false}, {'/', 0x38, false},
  {':', 0x33, true},  {';', 0x33, false}, {'<', 0x36, true},  {'=', 0x2E, false},
  {'>', 0x37, true},  {'?', 0x38, true},  {'@', 0x1F, true},  {'[', 0x2F, false},
  {'\\', 0x31, false},{']', 0x30, false}, {'^', 0x23, true},  {'_', 0x2D, true},
  {'`', 0x35, false}, {'{', 0x2F, true},  {'|', 0x31, true},  {'}', 0x30, true},
  {'~', 0x35, true},
};

inline KeyStroke asciiToKey(char c) {
  if (c >= 'a' && c <= 'z') return {0, (uint8_t)(0x04 + (c - 'a'))};
  if (c >= 'A' && c <= 'Z') return {KEY_MOD_LSHIFT, (uint8_t)(0x04 + (c - 'A'))};
  if (c >= '1' && c <= '9') return {0, (uint8_t)(0x1E + (c - '1'))};
  if (c == '0') return {0, 0x27};
  if (c == '\n') return {0, HID_KEY_ENTER};
  if (c == '\t') return {0, HID_KEY_TAB};
  for (const PunctKey &k : PUNCT_KEYS) {
    if (k.c == c) return {k.shift ? (uint8_t)KEY_MOD_LSHIFT : (uint8_t)0, k.code};
  }
  return {0, 0};
}
