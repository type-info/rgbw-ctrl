#pragma once

#include <Arduino.h>

class Sensor
{
private:
    gpio_num_t pin;

public:
    explicit Sensor(const gpio_num_t pin) : pin(pin)
    {
        pinMode(pin, INPUT);
    }

    [[nodiscard]] float readVoltage() const
    {
        const auto millis = analogReadMilliVolts(this->pin);
        return millis * 23.05f / 2114.0f;
    }
};
