## 💡 Light Class

The `Light` class represents a PWM control channel for LEDs (e.g., RGBW), with support for:

* Brightness control (0 to 255)
* On/off state with automatic persistence
* Storage in `Preferences` per GPIO pin
* Brightness adjustment with perceptual gamma curve (gamma 2.2)
* Optional inverted PWM signal
* JSON interface for external integration

### Features

* Uses `ledcWrite` at 25 kHz frequency with 8-bit resolution
* Persists state across reboots using unique keys per pin
* Avoids repeated writes with asynchronous debounce logic
* Simple API for setting state and value

### Key Methods

* `handle()` — Must be called periodically to persist state changes (debounced)
* `setup()` — Initializes the pin, configures PWM, and restores saved state
* `setValue(uint8_t)` — Sets the brightness and toggles on/off accordingly
* `setState(bool)` — Turns the light on or off, retaining the current brightness
* `toggle()` — Switches between on and off states
* `increaseBrightness()` / `decreaseBrightness()` — Adjusts brightness perceptually
* `resetPreferences()` — Clears the persisted state
* `toJson(JsonObject&)` — Exports the current state as JSON
