#pragma once

#include "color.hh"
#include "light.hh"
#include "hardware.hh"

#include <array>
#include <Arduino.h>
#include <algorithm>

class Output
{
    Light red = Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::RED)));
    Light green = Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::GREEN)));
    Light blue = Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::BLUE)));
    Light white = Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::WHITE)));

    std::array<Light*, 4> lights = {&red, &green, &blue, &white};

    static_assert(static_cast<size_t>(Color::White) < 4, "Color enum out of bounds");

public:
    void begin() const
    {
        for (auto& light : lights)
            light->setup();
    }

    void update(Color color, uint8_t value)
    {
        lights.at(static_cast<size_t>(color))->setValue(value);
    }

    bool getState(Color color) const
    {
        return lights.at(static_cast<size_t>(color))->isOn();
    }

    uint8_t getValue(Color color) const
    {
        return lights.at(static_cast<size_t>(color))->getValue();
    }

    void toggle(Color color)
    {
        lights.at(static_cast<size_t>(color))->toggle();
    }

    void updateAll(const uint8_t value)
    {
        for (auto& light : lights)
            light->setValue(value);
    }

    void toggleAll()
    {
        const bool anyOn = std::any_of(lights.begin(), lights.end(),
                                       [](const Light* light) { return light->isOn(); });
        for (auto& light : lights)
        {
            if (anyOn)
                light->setState(false);
            else
                light->setValue(Light::ON_VALUE);
        }
    }

    void increaseBrightness()
    {
        for (auto& light : lights)
            light->increaseBrightness();
    }

    void decreaseBrightness()
    {
        for (auto& light : lights)
            light->decreaseBrightness();
    }

    void turnOff()
    {
        for (auto& light : lights)
            light->setState(false);
    }

    void turnOn()
    {
        for (auto& light : lights)
            light->setValue(Light::ON_VALUE);
    }

    void setColor(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t w = 0)
    {
        red.setValue(r);
        green.setValue(g);
        blue.setValue(b);
        white.setValue(w);
    }

    [[nodiscard]] uint8_t getColor(Color color) const
    {
        return lights.at(static_cast<size_t>(color))->getValue();
    }
};
