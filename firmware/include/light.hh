#pragma once

#include <Arduino.h>
#include <Preferences.h>
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

    char valueKey[5]{};
    char stateKey[5]{};

    mutable std::optional<uint8_t> lastWrittenValue = std::nullopt;
    mutable bool lastPersistedState = false;
    mutable uint8_t lastPersistedValue = 255;

    void update() const
    {
        const auto& channel = Hardware::PWM_CHANNELS.at(this->pin);
        const auto duty = this->state ? this->value : OFF_VALUE;
        uint8_t outputValue = invert ? ON_VALUE - duty : duty;

        if (!lastWrittenValue  || outputValue != lastWrittenValue)
        {
            ledcWrite(channel, outputValue);
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

public:
    explicit Light(const gpio_num_t pin, const bool invert = false) :
        invert(invert), state(false), value(OFF_VALUE), pin(pin)
    {
        snprintf(stateKey, sizeof(stateKey), "%02us", static_cast<unsigned>(pin));
        snprintf(valueKey, sizeof(valueKey), "%02uv", static_cast<unsigned>(pin));
    }

    void setup()
    {
        const auto& channel = Hardware::PWM_CHANNELS.at(pin);
        pinMode(pin, OUTPUT);
        ledcSetup(channel, PWM_FREQUENCY, PWM_RESOLUTION);
        ledcAttachPin(pin, channel);
        restore();
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

    void toggle(uint8_t value)
    {
        this->value = value;
        this->state = value > 0;
        this->update();
    }

    void setValue(uint8_t value)
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
        this->value = this->value < ON_VALUE - 10 ? this->value + 10 : ON_VALUE;
        this->state = true;
        this->update();
    }

    void decreaseBrightness()
    {
        this->value = this->value > 10 ? this->value - 10 : OFF_VALUE;
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
