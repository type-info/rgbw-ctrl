#pragma once

#include "color.hh"
#include "light.hh"
#include "hardware.hh"

#include <array>
#include <Arduino.h>
#include <algorithm>
#include <webserver_handler.hh>

class Output
{
    std::array<Light, 4> lights = {
        Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::RED))),
        Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::GREEN))),
        Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::BLUE))),
        Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::WHITE)))
    };

    static_assert(static_cast<size_t>(Color::White) < 4, "Color enum out of bounds");

public:
    void begin(WebServerHandler& webServerHandler)
    {
        for (auto& light : lights)
            light.setup();
        webServerHandler.on("/color", HTTP_GET, [this](AsyncWebServerRequest* request)
        {
            if (request->hasParam("r") && request->hasParam("g") && request->hasParam("b"))
            {
                auto r = this->getValue(Color::Red);

                if (request->hasParam("r"))
                {
                    r = request->getParam("r")->value().toInt();
                }
                auto g = this->getValue(Color::Green);
                if (request->hasParam("g"))
                {
                    g = request->getParam("g")->value().toInt();
                }
                auto b = this->getValue(Color::Blue);
                if (request->hasParam("b"))
                {
                    b = request->getParam("b")->value().toInt();
                }
                auto w = this->getValue(Color::White);
                if (request->hasParam("w"))
                {
                    w = request->getParam("w")->value().toInt();
                }
                this->setColor(r, g, b, w);
            }
            request->send(200, "text/plain", "Color set");
        });
    }

    void update(Color color, const uint8_t value)
    {
        lights.at(static_cast<size_t>(color)).setValue(value);
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
    }

    void updateAll(const uint8_t value)
    {
        for (auto& light : lights)
            light.setValue(value);
    }

    void toggleAll()
    {
        const bool anyOn = std::any_of(lights.begin(), lights.end(),
                                       [](const Light& light) { return light.isOn(); });
        for (auto& light : lights)
        {
            if (anyOn)
                light.setState(false);
            else
                light.setValue(Light::ON_VALUE);
        }
    }

    void increaseBrightness()
    {
        for (auto& light : lights)
            light.increaseBrightness();
    }

    void decreaseBrightness()
    {
        for (auto& light : lights)
            light.decreaseBrightness();
    }

    void turnOff()
    {
        for (auto& light : lights)
            light.setState(false);
    }

    void turnOn()
    {
        for (auto& light : lights)
            light.setValue(Light::ON_VALUE);
    }

    void setColor(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t w = 0)
    {
        lights.at(static_cast<size_t>(Color::Red)).setValue(r);
        lights.at(static_cast<size_t>(Color::Green)).setValue(g);
        lights.at(static_cast<size_t>(Color::Blue)).setValue(b);
        lights.at(static_cast<size_t>(Color::White)).setValue(w);
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

    void setValues(const std::array<uint8_t, 4>& array)
    {
        for (size_t i = 0; i < std::min(lights.size(), array.size()); ++i)
        {
            update(static_cast<Color>(i), array[i]);
        }
    }
};
