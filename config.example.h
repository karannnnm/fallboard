// Copy this file to config.h and change every value before flashing.
// config.h is gitignored so your secrets never get committed.
#pragma once

// Name the Mac shows in Bluetooth settings.
#define BLE_DEVICE_NAME "Rescue Keyboard"

// Wi-Fi network the ESP32 creates. Password must be at least 8 characters.
#define AP_SSID     "rescue-kbd"
#define AP_PASSWORD "change-me-wifi"

// PIN for the web page. Separate from the Wi-Fi password on purpose:
// anyone who knows the Wi-Fi password still can't type into the Mac without it.
#define WEB_PIN "change-me-pin"

// Lockout after too many wrong PINs. Each lockout doubles, up to the max.
#define MAX_FAILED_ATTEMPTS 5
#define LOCKOUT_BASE_SECONDS 60
#define LOCKOUT_MAX_SECONDS 3600

// A logged-in session expires after this much inactivity.
#define SESSION_IDLE_SECONDS 300
