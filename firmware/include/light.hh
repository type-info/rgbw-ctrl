#pragma once

#include <Arduino.h>
#include "hardware.hh"

class Light
{
public:
    static constexpr uint8_t ON_VALUE = 255;
    static constexpr uint8_t OFF_VALUE = 0;

private:
    static constexpr uint32_t PWM_FREQUENCY = 5000;
    static constexpr uint8_t PWM_RESOLUTION = 8;

    bool invert;
    bool state;
    uint8_t value;
    gpio_num_t pin;

    void update() const
    {
        const auto& channel = Hardware::PWM_CHANNELS.at(this->pin);
        auto duty = this->state ? this->value : OFF_VALUE;
        ledcWrite(channel, invert ? ON_VALUE - duty : duty);
    }

public:
    explicit Light(gpio_num_t pin, bool invert = false) :
        invert(invert), state(false), value(OFF_VALUE), pin(pin)
    {
    }

    void setup() const
    {
        const auto& channel = Hardware::PWM_CHANNELS.at(pin);
        pinMode(pin, OUTPUT);
        ledcSetup(channel, PWM_FREQUENCY, PWM_RESOLUTION);
        ledcAttachPin(pin, channel);
        ledcWrite(channel, invert ? ON_VALUE : OFF_VALUE);
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

    [[nodiscard]] bool isOn() const { return state; }
    [[nodiscard]] uint8_t getValue() const { return value; }
};
