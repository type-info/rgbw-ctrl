#pragma once

#include "color.hh"
#include "light.hh"
#include "hardware.hh"

#include <array>
#include <Arduino.h>
#include <algorithm>
#include <functional>

class Output
{
    std::array<Light, 4> lights = {
        Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::RED))),
        Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::GREEN))),
        Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::BLUE))),
        Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::WHITE)))
    };

    std::function<void()> notifyBleCallback;

    static_assert(static_cast<size_t>(Color::White) < 4, "Color enum out of bounds");

    void notifyChange(const bool notifyBle = true) const
    {
        if (notifyBle && notifyBleCallback)
        {
            notifyBleCallback();
        }
    }

public:
    [[nodiscard]] bool anyOn() const
    {
        return std::any_of(lights.begin(), lights.end(),
                           [](const Light& light) { return light.isOn(); });
    }

    void begin()
    {
        for (auto& light : lights)
            light.setup();
    }

    void handle(const unsigned long now)
    {
        for (auto& light : lights)
        {
            light.handle(now);
        }
    }

    void setNotifyBleCallback(const std::function<void()>& callback)
    {
        notifyBleCallback = callback;
    }

    void update(Color color, const uint8_t value, const bool notifyBle = true)
    {
        lights.at(static_cast<size_t>(color)).setValue(value);
        notifyChange(notifyBle);
    }

    std::array<LightState, 4> getState() const
    {
        std::array<LightState, 4> state;
        std::transform(lights.begin(), lights.end(), state.begin(),
                       [](const Light& light) { return light.getState(); });
        return state;
    }

    void setState(const std::array<LightState, 4> state)
    {
        for (uint8_t i = 0; i < 4; ++i)
            lights[i].setState(state[i]);
    }

    [[nodiscard]] bool getState(Color color) const
    {
        return lights.at(static_cast<size_t>(color)).isOn();
    }

    [[nodiscard]] uint8_t getValue(Color color) const
    {
        return lights.at(static_cast<size_t>(color)).getValue();
    }

    void toggle(Color color)
    {
        lights.at(static_cast<size_t>(color)).toggle();
        notifyChange();
    }

    void updateAll(const uint8_t value)
    {
        for (auto& light : lights)
            light.setValue(value);
        notifyChange();
    }

    void toggleAll()
    {
        const bool on = anyOn();
        for (auto& light : lights)
        {
            if (on)
                light.setState(false);
            else
                light.setValue(Light::ON_VALUE);
        }
        notifyChange();
    }

    void increaseBrightness()
    {
        for (auto& light : lights)
            light.increaseBrightness();
        notifyChange();
    }

    void decreaseBrightness()
    {
        for (auto& light : lights)
            light.decreaseBrightness();
        notifyChange();
    }

    void turnOff()
    {
        for (auto& light : lights)
            light.setState(false);
        notifyChange();
    }

    void turnOn()
    {
        for (auto& light : lights)
            light.setValue(Light::ON_VALUE);
        notifyChange();
    }

    void setColor(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t w = 0)
    {
        lights.at(static_cast<size_t>(Color::Red)).setValue(r);
        lights.at(static_cast<size_t>(Color::Green)).setValue(g);
        lights.at(static_cast<size_t>(Color::Blue)).setValue(b);
        lights.at(static_cast<size_t>(Color::White)).setValue(w);
        notifyChange();
    }

    [[nodiscard]] std::array<uint8_t, 4> getValues() const
    {
        std::array<uint8_t, 4> output = {};
        std::transform(lights.begin(), lights.end(), output.begin(), [](const auto& light)
        {
            return light.getValue();
        });
        return output;
    }

    void setValues(const std::array<uint8_t, 4>& array, const bool notifyBle = true)
    {
        for (size_t i = 0; i < std::min(lights.size(), array.size()); ++i)
        {
            update(static_cast<Color>(i), array[i], notifyBle);
        }
    }

    void toJson(const JsonArray& to) const
    {
        for (const auto& light : lights)
        {
            auto obj = to.add<JsonObject>();
            light.toJson(obj);
        }
    }
};
