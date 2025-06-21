# OTA Update Documentation – RGBW Controller

This document provides a standalone technical reference for the OTA (Over-The-Air) update process implemented in the ESP32-based RGBW Controller firmware.

---

## Overview

The OTA update process allows firmware and filesystem images to be uploaded to the device over HTTP. The logic is encapsulated in the `OtaHandler` class and executed through a custom `AsyncWebHandler`, providing a secure and robust flow.

Supported updates:

* 🔧 Firmware (`U_FLASH`)
* 📁 Filesystem (`U_SPIFFS` for LittleFS partitions)

Multiple requests to the `/update` endpoint are allowed concurrently, but only the first one will be processed. All others will be rejected with an appropriate error. This ensures the system remains robust under concurrent access.

---

## Authentication

All OTA requests are protected using Basic Authentication.
If the request does not contain valid credentials, it is rejected with:

```
HTTP/1.1 401 Unauthorized
```

The `AsyncAuthenticationMiddleware` handles access control.

---

## Endpoint

### `POST /update`

| Parameter | Type   | Required  | Description                          |
| --------- | ------ | --------- | ------------------------------------ |
| `file`    | file   | ✅ Yes    | Binary file (firmware or filesystem) |
| `name`    | string | ❌ No     | `filesystem` (default is firmware)   |
| `md5`     | string | ❌ No     | 32-char hex string to validate file  |

Example (firmware):

```bash
curl -u user:pass -F "file=@firmware.bin" http://<device-ip>/update
```

Example (filesystem):

```bash
curl -u user:pass -F "name=filesystem" -F "file=@littlefs.bin" http://<device-ip>/update
```

Example with MD5:

```bash
curl -u user:pass -F "file=@firmware.bin" "http://<device-ip>/update?md5=d41d8cd98f00b204e9800998ecf8427e"
```

---

## MD5 Integrity Check (Optional but Recommended)

To ensure the integrity of the uploaded file, the OTA handler supports an optional `md5` query parameter. This value is compared against the MD5 checksum calculated during the upload process. If they do not match, the update is aborted and an error is returned.

### ✅ How to use

Add the `md5` parameter to your request:

```bash
curl -u user:pass -F "file=@firmware.bin" "http://<device-ip>/update?md5=d41d8cd98f00b204e9800998ecf8427e"
```

You may also combine with the `name` parameter:

```bash
curl -u user:pass -F "file=@littlefs.bin" "http://<device-ip>/update?name=filesystem&md5=d41d8cd98f00b204e9800998ecf8427e"
```

### 🛠 Behavior

- If provided, the handler calls `Update.setMD5()` before `Update.begin()`.
- If the hash is invalid or doesn't match, `Update.end(true)` will fail.
- A 500 error will be returned to the client, with a message such as:
  ```
  MD5 mismatch
  ```

### ⚠️ Note

- The MD5 must be a 32-character hexadecimal string.
- If the parameter is omitted, the firmware will proceed without integrity validation.

---

## Auto-Restart

Upon a successful update (`Update.end(true)`), the device calls `ESP.restart()` to apply the new firmware or filesystem.
To ensure the response reaches the client first, the restart is deferred using:

```cpp
request->onDisconnect([]() {
    ESP.restart();
});
```

---

## Error Scenarios

| Case                          | Response Code | Notes                                |
| ----------------------------- | ------------- | ------------------------------------ |
| No authentication             | 401           | Requires Basic Auth                  |
| OTA already in progress       | 400           | Double request protection            |
| Update.begin() failed         | 500           | Flash not available or corrupt       |
| Upload incomplete             | 500           | Did not receive full file            |
| Update.write() failed mid-way | 500           | Flash write error, possibly hardware |
| MD5 mismatch                  | 500           | Checksum invalid or corrupt file     |

---

## Progress and Status Retrieval

Instead of receiving progress callbacks, consumers must periodically poll the update state using:

```cpp
OtaHandler::getState(); // Returns an OtaState struct with status, totalBytesExpected, totalBytesReceived
```

Or if only the status is needed:

```cpp
OtaHandler::getStatus(); // Returns OtaStatus enum value
```

The `OtaState` struct includes the following fields:

```cpp
struct OtaState {
    OtaStatus status;
    uint32_t totalBytesExpected;
    uint32_t totalBytesReceived;
};
```

This approach avoids coupling to asynchronous callbacks and allows consistent polling through BLE, WebSocket, or REST.

---

## Error Message Truncation

OTA errors returned to the client (via HTTP 500) are truncated to a maximum of 64 bytes to ensure safe delivery and memory efficiency.

---

## Thread Safety and Concurrency

Internally, the update state is tracked using `std::atomic` to ensure thread-safe transitions and consistent reads across callbacks. Only the first valid OTA request is accepted and processed; subsequent simultaneous requests are safely rejected with a 400 response.

Monitoring variables such as `totalBytesExpected` and `totalBytesReceived` are declared as `volatile` to reflect real-time progress. These are suitable for passive reads, such as progress indicators.

---

## Conclusion

The OTA handler offers a safe and extensible mechanism to update the ESP32 remotely, with proper authentication, state control, and progress monitoring. It integrates tightly with Bluetooth status and provides detailed error handling to cover edge cases.
