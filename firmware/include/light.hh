#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <cmath>

#include "hardware.hh"

#pragma pack(push, 1)
struct LightState
{
    bool enabled = false;
    uint8_t value = 0;

    bool operator ==(const LightState& other) const
    {
        return (enabled == other.enabled && value == other.value);
    }

    bool operator !=(const LightState& other) const
    {
        return !(*this == other);
    }

    void toJson(const JsonObject& to) const
    {
        to["state"] = enabled ? "on" : "off";
        to["value"] = value;
    }
};
#pragma pack(pop)

class Light
{
public:
    static constexpr uint8_t ON_VALUE = 255;
    static constexpr uint8_t OFF_VALUE = 0;
    static constexpr auto PREFERENCES_NAME = "light";

    void setup()
    {
        prefs.begin(PREFERENCES_NAME, false);
        if (const auto& channel = Hardware::getPwmChannel(pin))
        {
            pinMode(pin, OUTPUT);
            ledcSetup(channel.value(), PWM_FREQUENCY, PWM_RESOLUTION);
            ledcAttachPin(pin, channel.value());
            restore();
        }
        else
        {
            ESP_LOGE("Light", "Invalid pin %d for PWM channel", static_cast<int>(pin));
        }
    }

    void handle(const unsigned long now)
    {
        if (state != lastPersistedState && now - lastPersistTime >= PERSIST_DEBOUNCE_MS)
        {
            prefs.putBool(stateKey, state.enabled);
            prefs.putUChar(valueKey, state.value);
            lastPersistedState = state;
            lastPersistTime = now;
        }
    }

private:
    static constexpr uint32_t PWM_FREQUENCY = 25000;
    static constexpr uint8_t PWM_RESOLUTION = 8;
    static constexpr unsigned long PERSIST_DEBOUNCE_MS = 500;

    bool invert;
    gpio_num_t pin;
    LightState state;

    char valueKey[5] = "";
    char stateKey[5] = "";

    std::optional<uint8_t> lastWrittenValue = std::nullopt;

    Preferences prefs;
    LightState lastPersistedState;
    unsigned long lastPersistTime = 0;

    void update()
    {
        const auto& channel = Hardware::getPwmChannel(pin);
        const auto duty = state.enabled ? state.value : OFF_VALUE;

        if (uint8_t outputValue = invert ? ON_VALUE - duty : duty;
            !lastWrittenValue || outputValue != lastWrittenValue)
        {
            ledcWrite(channel.value(), outputValue);
            lastWrittenValue = outputValue;
        }
    }

    void restore()
    {
        state.enabled = prefs.getBool(stateKey, false);
        state.value = prefs.getUChar(valueKey, OFF_VALUE);
        update();
    }

    static uint8_t perceptualBrightnessStep(const uint8_t currentValue, const bool increase)
    {
        constexpr float gamma = 2.2f;
        float linear = pow(static_cast<float>(currentValue) / 255.0f, 1.0f / gamma);
        linear += (increase ? 0.05f : -0.05f);
        linear = std::clamp(linear, 0.0f, 1.0f);
        return static_cast<uint8_t>(lround(pow(linear, gamma) * 255.0f));
    }

public:
    explicit Light(const gpio_num_t pin, const bool invert = false) :
        invert(invert), pin(pin)
    {
        snprintf(stateKey, sizeof(stateKey), "%02us", static_cast<unsigned>(pin));
        snprintf(valueKey, sizeof(valueKey), "%02uv", static_cast<unsigned>(pin));
    }

    ~Light()
    {
        prefs.end();
    }

    void toggle()
    {
        state.enabled = !state.enabled;
        if (state.enabled && state.value == OFF_VALUE)
        {
            state.value = ON_VALUE;
        }
        update();
    }

    void setValue(const uint8_t value)
    {
        state.value = value;
        if (value > OFF_VALUE && !state.enabled)
            state.enabled = true;
        if (value == OFF_VALUE)
            state.enabled = false;
        update();
    }

    void setState(const bool stateFlag)
    {
        state.enabled = stateFlag;
        if (stateFlag && state.value == OFF_VALUE)
        {
            state.value = ON_VALUE;
        }
        update();
    }

    void increaseBrightness()
    {
        state.value = perceptualBrightnessStep(state.value, true);
        state.enabled = true;
        update();
    }

    void decreaseBrightness()
    {
        state.value = perceptualBrightnessStep(state.value, false);
        if (state.value == OFF_VALUE)
            state.enabled = false;
        update();
    }

    void toJson(const JsonObject& to) const
    {
        state.toJson(to);
    }

    [[nodiscard]] bool isOn() const { return state.enabled; }
    [[nodiscard]] uint8_t getValue() const { return state.value; }
    [[nodiscard]] LightState getState() const { return state; }
};
