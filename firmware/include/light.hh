#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <cmath>
#include <cstdlib>
#include "hardware.hh"

class Light
{
public:
    static constexpr uint8_t ON_VALUE = 255;
    static constexpr uint8_t OFF_VALUE = 0;

private:
    static constexpr uint32_t PWM_FREQUENCY = 25000;
    static constexpr uint8_t PWM_RESOLUTION = 8;

    bool invert;
    bool state;
    uint8_t value;
    gpio_num_t pin;

    char valueKey[5] = "";
    char stateKey[5] = "";

    std::optional<uint8_t> lastWrittenValue = std::nullopt;
    bool lastPersistedState = false;
    uint8_t lastPersistedValue = 255;

    void update()
    {
        const auto& channel = Hardware::getPwmChannel(this->pin);
        const auto duty = this->state ? this->value : OFF_VALUE;

        if (uint8_t outputValue = invert ? ON_VALUE - duty : duty;
            !lastWrittenValue || outputValue != lastWrittenValue)
        {
            ledcWrite(channel.value(), outputValue);
            lastWrittenValue = outputValue;
        }

        if (state != lastPersistedState || value != lastPersistedValue)
        {
            persist();
            lastPersistedState = state;
            lastPersistedValue = value;
        }
    }

    void persist() const
    {
        Preferences prefs;
        prefs.begin("light", false);
        prefs.putBool(stateKey, state);
        prefs.putUChar(valueKey, value);
        prefs.end();
    }

    void restore()
    {
        Preferences prefs;
        prefs.begin("light", true);
        state = prefs.getBool(stateKey, false);
        value = prefs.getUChar(valueKey, OFF_VALUE);
        prefs.end();
        lastPersistedState = state;
        lastPersistedValue = value;
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
        invert(invert), state(false), value(OFF_VALUE), pin(pin)
    {
        snprintf(stateKey, sizeof(stateKey), "%02us", static_cast<unsigned>(pin));
        snprintf(valueKey, sizeof(valueKey), "%02uv", static_cast<unsigned>(pin));
    }

    void setup()
    {
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

    void toggle()
    {
        this->state = !this->state;
        if (this->state && this->value == OFF_VALUE)
        {
            this->value = ON_VALUE;
        }
        this->update();
    }

    void toggle(const uint8_t value)
    {
        this->value = value;
        this->state = value > 0;
        this->update();
    }

    void setValue(const uint8_t value)
    {
        this->value = value;
        if (value > OFF_VALUE && !this->state)
            this->state = true;
        if (value == OFF_VALUE)
            this->state = false;
        this->update();
    }

    void setValue(const bool state, const uint8_t value)
    {
        this->state = state;
        this->value = value;
        this->update();
    }

    void setState(const bool state)
    {
        this->state = state;
        if (state && this->value == OFF_VALUE)
        {
            this->value = ON_VALUE;
        }
        this->update();
    }

    void increaseBrightness()
    {
        this->value = perceptualBrightnessStep(this->value, true);
        this->state = true;
        this->update();
    }

    void decreaseBrightness()
    {
        this->value = perceptualBrightnessStep(this->value, false);
        if (this->value == OFF_VALUE)
            this->state = false;
        this->update();
    }

    void resetPreferences() const
    {
        Preferences prefs;
        prefs.begin("light", false);
        prefs.remove(stateKey);
        prefs.remove(valueKey);
        prefs.end();
    }

    [[nodiscard]] bool isOn() const { return state; }
    [[nodiscard]] uint8_t getValue() const { return value; }
};
