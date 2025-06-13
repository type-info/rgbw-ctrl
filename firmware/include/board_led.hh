#pragma once

#include <array>
#include <Arduino.h>

#include "color.hh"
#include "light.hh"
#include "hardware.hh"
#include "ble_manager.hh"
#include "wifi_model.hh"

class BoardLED
{
private:
    Light red = Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::BoardLed::RED)), true);
    Light green = Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::BoardLed::GREEN)), true);
    Light blue = Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::BoardLed::BLUE)), true);

    std::array<Light*, 3> leds = {&red, &green, &blue};

    static_assert(static_cast<size_t>(Color::Blue) < 3, "Color enum out of bounds for BoardLED");

public:
    void begin()
    {
        for (const auto& led : leds)
            led->setup();
    }

    void handle(const bool isBleActive, const bool isBleClientConnected, const WifiScanStatus wifiScanStatus,
                const WiFiStatus wifiStatus)
    {
        static unsigned long lastBlinkTime = 0;
        static bool ledOn = false;

        const unsigned long now = millis();

        // Steady yellow: connected to a BLE client
        if (isBleClientConnected)
        {
            this->setColor(32, 32, 0);
        }
        // Blinking blue: advertising, but not connected to a client
        else if (isBleActive)
        {
            if (now - lastBlinkTime > 400)
            {
                lastBlinkTime = now;
                ledOn = !ledOn;
            }
            this->setColor(0, 0, ledOn ? 32 : 0); // Blink blue
        }
        // Blinking yellow: Wi-Fi scan running
        else if (wifiScanStatus == WifiScanStatus::RUNNING)
        {
            if (now - lastBlinkTime > 400)
            {
                lastBlinkTime = now;
                ledOn = !ledOn;
            }
            this->setColor(ledOn ? 32 : 0, ledOn ? 32 : 0, 0); // Blink yellow
        }
        // Steady green: connected to Wi-Fi
        else if (wifiStatus == WiFiStatus::CONNECTED)
        {
            this->setColor(0, 32, 0);
        }
        // Steady red: not connected to Wi-Fi
        else
        {
            this->setColor(32, 0, 0);
        }
    }

private:
    void update(Color color, const uint8_t value)
    {
        leds.at(static_cast<size_t>(color))->setValue(value);
    }

    void toggle(Color color)
    {
        if (color == Color::White)
        {
            red.toggle();
            green.toggle();
            blue.toggle();
            return;
        }
        leds.at(static_cast<size_t>(color))->toggle();
    }

    void updateAll(const uint8_t value)
    {
        for (const auto& led : leds)
            led->setValue(value);
    }

    void toggleAll()
    {
        for (const auto& led : leds)
            led->toggle();
    }

    void increaseBrightness()
    {
        for (const auto& led : leds)
            led->increaseBrightness();
    }

    void decreaseBrightness()
    {
        for (const auto& led : leds)
            led->decreaseBrightness();
    }

    void turnOff()
    {
        for (const auto& led : leds)
            led->setState(false);
    }

    void setColor(const uint8_t r, const uint8_t g, const uint8_t b)
    {
        red.setValue(r);
        green.setValue(g);
        blue.setValue(b);
    }

    [[nodiscard]] uint8_t getColor(Color color) const
    {
        return leds.at(static_cast<size_t>(color))->getValue();
    }
};
