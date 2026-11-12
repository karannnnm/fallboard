# fallboard

An ESP32 that acts as a Bluetooth keyboard + mouse for your Mac, controlled from your phone over Wi-Fi. A fallback for when your real keyboard dies, including at the login and FileVault screens.

## Setup

```sh
cp config.example.h config.h   # set your Wi-Fi password and PIN
arduino-cli lib install "ESP Async WebServer" "Async TCP" "NimBLE-Arduino"
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app .
arduino-cli upload  --fqbn esp32:esp32:esp32:PartitionScheme=huge_app -p /dev/cu.usbserial-XXXX .
```

Pair **Rescue Keyboard** once in the Mac's Bluetooth settings. Then join the `rescue-kbd` Wi-Fi on your phone, open http://192.168.4.1 and enter your PIN.
