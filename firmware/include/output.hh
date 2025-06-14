#pragma once

#include "color.hh"
#include "light.hh"
#include "hardware.hh"

#include <array>
#include <Arduino.h>
#include <algorithm>
#include <functional>

struct OutputState
{
    std::array<bool, 4> states;
    std::array<uint8_t, 4> values;

    bool operator==(const OutputState& output_state) const
    {
        return std::equal(states.begin(), states.end(), output_state.states.begin())
            && std::equal(values.begin(), values.end(), output_state.values.begin());
    }
};

class Output
{
    Light red = Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::RED)));
    Light green = Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::GREEN)));
    Light blue = Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::BLUE)));
    Light white = Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::WHITE)));

    std::array<Light*, 4> lights = {&red, &green, &blue, &white};
    std::function<void(const OutputState&)> onChangeCallback;
    OutputState lastOutputState = {};

    static_assert(static_cast<size_t>(Color::White) < 4, "Color enum out of bounds");

    void notifyChange()
    {
        if (onChangeCallback)
        {
            const OutputState state = {
                {red.isOn(), green.isOn(), blue.isOn(), white.isOn()},
                {red.getValue(), green.getValue(), blue.getValue(), white.getValue()}
            };
            if (state == lastOutputState) return;
            lastOutputState = state;
            onChangeCallback(state);
        }
    }

public:
    void setOnChangeCallback(const std::function<void(const OutputState&)>& callback)
    {
        onChangeCallback = callback;
    }

    void begin() const
    {
        for (auto& light : lights)
            light->setup();
    }

    void update(Color color, uint8_t value)
    {
        lights.at(static_cast<size_t>(color))->setValue(value);
        notifyChange();
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
        notifyChange();
    }

    void updateAll(const uint8_t value)
    {
        for (auto& light : lights)
            light->setValue(value);
        notifyChange();
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
        notifyChange();
    }

    void increaseBrightness()
    {
        for (auto& light : lights)
            light->increaseBrightness();
        notifyChange();
    }

    void decreaseBrightness()
    {
        for (auto& light : lights)
            light->decreaseBrightness();
        notifyChange();
    }

    void turnOff()
    {
        for (auto& light : lights)
            light->setState(false);
        notifyChange();
    }

    void turnOn()
    {
        for (auto& light : lights)
            light->setValue(Light::ON_VALUE);
        notifyChange();
    }

    void setColor(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t w = 0)
    {
        red.setValue(r);
        green.setValue(g);
        blue.setValue(b);
        white.setValue(w);
        notifyChange();
    }

    [[nodiscard]] uint8_t getColor(Color color) const
    {
        return lights.at(static_cast<size_t>(color))->getValue();
    }
};
