# RGBW Controller

This repository provides the firmware for an ESP32 based RGBW LED controller together with an Angular web application for configuration. The device exposes Wi‑Fi and Bluetooth Low Energy services so it can be configured directly from a browser via Web Bluetooth. It also supports Alexa integration and OTA firmware updates.

## BOOT Button Behavior
The BOOT button on the board has multiple context-sensitive behaviors:

Short Press (< 2.5s)
🟢 Toggles the board's lights on or off, depending on the current state.

Long Press (> 2.5s)
🔵 Enables Bluetooth, if it’s not already active.
🔄 If Bluetooth is already on, it simply resets its 15s auto-shutdown timer.

⏱️ Bluetooth automatically disables after 15 seconds if no connection is established.

Firmware Update Mode
🛠️ To enter UART firmware update mode:

Hold the BOOT button,

Then press RESET.

## Board LED Status Indicator

The `BoardLED` class provides a simple, visual representation of system status using the onboard RGB LED. It conveys Bluetooth, Wi-Fi, and OTA update states using colors and effects (steady or fading).

### Features

* ✨ **Smooth fade blinking** for advertising and scanning states
* 🟢 **Static colors** for stable statuses
* 🎯 **Priority handling** to ensure the most critical state is always shown

### Color Codes & Behavior

| State                   | Color      | Behavior | Description                             |
| ----------------------- | ---------  | -------- | --------------------------------------- |
| 🔄 OTA update running   | 🟣 Purple | Fading   | Indicates firmware update in progress   |
| 🤝 BLE client connected | 🟡 Yellow | Steady   | Device is actively connected via BLE    |
| 📡 BLE advertising      | 🔵 Blue   | Fading   | BLE is active, waiting for a connection |
| 📶 Wi-Fi scan running   | 🟡 Yellow | Fading   | Scanning for available Wi-Fi networks   |
| 🌐 Wi-Fi connected      | 🟢 Green  | Steady   | Device is connected to a Wi-Fi network  |
| ❌ Wi-Fi disconnected   | 🔴 Red    | Steady   | No Wi-Fi connection available           |

## OTA Update via Web Server

This project includes support for OTA (Over-the-Air) firmware and filesystem updates via HTTP POST requests using the `OtaHandler` class.

### 📡 Supported Upload Targets

* **Firmware**
* **Filesystem** (works only with LittleFS partitions)

---

### 🔒 Authentication

OTA endpoints are protected using Basic Authentication via `AsyncAuthenticationMiddleware`. Unauthorized requests receive `401 Unauthorized`.

---

### 🔧 Endpoints

#### `POST /update`

Uploads a new firmware or filesystem image.

##### Parameters

| Parameter | Type   | Required | Description                          |
| --------- | ------ | -------- | ------------------------------------ |
| `name`    | string | Optional | `firmware` (default) or `filesystem` |

##### Example

**Upload firmware:**

```bash
curl -u user:pass -F "file=@firmware.bin" http://<device-ip>/update
```

**Upload filesystem (LittleFS):**

```bash
curl -u user:pass -F "name=filesystem" -F "file=@littlefs.bin" http://<device-ip>/update
```

---

### 🔁 Auto-Restart

After a successful OTA update, the device automatically restarts using:

```cpp
ESP.restart();
```

This is triggered in a safe way via `request->onDisconnect(...)` to ensure the HTTP response is completed before reboot.


## License

```
MIT License

Copyright (c) 2025 type-info

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
```
The remainder of the license text can be found in the [LICENSE](LICENSE) file.
