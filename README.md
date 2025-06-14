# RGBW Controller

This repository provides the firmware for an ESP32 based RGBW LED controller together with an Angular web application for configuration. The device exposes Wi‑Fi and Bluetooth Low Energy services so it can be configured directly from a browser via Web Bluetooth. It also supports Alexa integration and OTA firmware updates.

## Features

- RGBW LED control with adjustable brightness and color
- Wi‑Fi configuration over Bluetooth through the PWA
- Device name customization and heap monitoring
- Alexa integration for voice control
- Over‑the‑air firmware updates
- HTTP endpoints to restart the controller and set colors

## Repository structure

- **`firmware/`** – PlatformIO project implementing the ESP32 firmware.
- **`app/`** – Angular PWA that communicates with the device over BLE.

## Building the firmware

Install [PlatformIO](https://platformio.org/) and then run:

```bash
cd firmware
pio run            # build
pio run -t upload  # build and upload to the board
```

Key build settings are defined in `firmware/platformio.ini`:

```ini
[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200
monitor_filters = direct
board_build.partitions = partitions.csv
lib_deps =
    ESP32Async/AsyncTCP
        ESP32Async/ESPAsyncWebServer
    ipdotsetaf/ESPAsyncHTTPUpdateServer
    https://github.com/type-info/Espalexa.git
build_unflags = -std=gnu++11
build_flags =
    -std=gnu++2a
    -D ESPALEXA_ASYNC
    -D CONFIG_BT_CONTROLLER_MODE_BLE_ONLY=1
    -D ESPASYNCHTTPUPDATESERVER_PRETTY
    -D CONFIG_ASYNC_TCP_MAX_ACK_TIME=10000
```

## Building the web application

The web interface requires Node.js and npm. To build the PWA run:

```bash
cd app
npm install
npm run build
```

The output will be generated in `app/dist/rgbw-ctrl-setup`. Available npm scripts are listed in `app/package.json`:

```json
{
  "name": "rgbw-ctrl-setup",
  "version": "0.0.0",
  "scripts": {
    "ng": "ng",
    "start": "ng serve",
    "build": "ng build",
    "watch": "ng build --watch --configuration development",
    "test": "ng test"
  }
}
```

During development you can run `npm start` to launch the dev server.

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
