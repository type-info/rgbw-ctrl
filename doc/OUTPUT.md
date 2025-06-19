## 🎛️ Output Class

The `Output` class is responsible for managing a group of four `Light` instances representing RGBW LED control. It abstracts grouped brightness control, state management, and notification of state changes to BLE or WebSocket layers.

### ✨ Features

* Controls 4 PWM-driven lights (Red, Green, Blue, White)
* Supports turning on/off individual or all channels
* Enables fine-grained brightness control (0–255)
* Debounced state persistence through `Light::handle()`
* Notifies BLE and WebSocket layers via callback hooks
* JSON serialization for integration

### 🧩 Integration

The `Output` class is decoupled from HTTP and BLE logic. Notification hooks can be set via:

```cpp
setNotifyBleCallback(...);
setNotifyWebSocketCallback(...);
```

### 📌 Usage

Initialize and periodically call `handle(millis())` to persist state changes:

```cpp
Output output;
output.begin();
output.setNotifyBleCallback(...);
output.setNotifyWebSocketCallback(...);

loop() {
    output.handle(millis());
}
```

### 🔧 Methods

* `begin()` — initializes all lights
* `handle(now)` — persists light states if changed (debounced)
* `update(color, value)` — sets brightness for a color
* `toggle(color)` — toggles a color on/off
* `updateAll(value)` — sets all channels to the same brightness
* `toggleAll()` — turns all on/off depending on current state
* `increaseBrightness()` / `decreaseBrightness()` — modifies all channels
* `turnOn()` / `turnOff()` — all channels on or off
* `setColor(r, g, b, w)` — sets RGBW values directly
* `getValues()` / `setValues(array)` — batch get/set values
* `toJson(JsonArray&)` — serializes light state to JSON

### 📥 State Query Helpers

* `getState(color)` — is the light on?
* `getValue(color)` — current brightness
* `anyOn()` — true if any light is on

### 🧠 Notes

* Call `handle()` regularly (e.g., in `loop()`) to ensure that brightness/state changes are saved persistently.
* Callback functions are optional but recommended to reflect real-time changes in external interfaces like BLE or Web UI.
