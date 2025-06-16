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
    static constexpr uint8_t MAX_BRIGHTNESS = 32;
    static constexpr unsigned long BLINK_INTERVAL_MS = 20;
    static constexpr int TRANSITION_STEP = 4;

    std::array<Light, 3> leds = {
        Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::BoardLed::RED)), true),
        Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::BoardLed::GREEN)), true),
        Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::BoardLed::BLUE)), true)
    };

    static_assert(static_cast<size_t>(Color::Blue) < 3, "Color enum out of bounds for BoardLED");

    unsigned long lastBlinkTime = 0;
    int fadeValue = 0;
    int fadeDirection = TRANSITION_STEP;

public:
    void begin()
    {
        for (auto& led : leds)
            led.setup();
    }

    void handle(const unsigned long now, const bool isBleActive,
                const bool isBleClientConnected, const WifiScanStatus wifiScanStatus,
                const WiFiStatus wifiStatus, const bool isOtaUpdateRunning)
    {
        // If OTA update is running, blink purple
        if (isOtaUpdateRunning)
        {
            const uint8_t value = getFadeValue(now);
            this->setColor({value, 0, value}); // Purple
            return;
        }
        // Steady yellow: connected to a BLE client
        if (isBleClientConnected)
        {
            this->setColor({MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0});
            return;
        }
        // Blinking blue: advertising, but not connected to a client
        if (isBleActive)
        {
            const uint8_t value = getFadeValue(now);
            this->setColor({0, 0, value});
            return;
        }
        // Blinking yellow: Wi-Fi scan running
        if (wifiScanStatus == WifiScanStatus::RUNNING)
        {
            const uint8_t value = getFadeValue(now);
            this->setColor({value, value, 0});
            return;
        }
        // Steady green: connected to Wi-Fi
        if (wifiStatus == WiFiStatus::CONNECTED)
        {
            this->setColor({0, MAX_BRIGHTNESS, 0});
            return;
        }
        // Steady red: not connected to Wi-Fi
        this->setColor({MAX_BRIGHTNESS, 0, 0});
    }

private:
    uint8_t getFadeValue(const unsigned long now)
    {
        if (now - lastBlinkTime >= BLINK_INTERVAL_MS)
        {
            lastBlinkTime = now;
            fadeValue += fadeDirection;

            if (fadeValue >= MAX_BRIGHTNESS)
            {
                fadeValue = MAX_BRIGHTNESS;
                fadeDirection = -TRANSITION_STEP;
            }
            else if (fadeValue <= 0)
            {
                fadeValue = 0;
                fadeDirection = TRANSITION_STEP;
            }
        }
        return static_cast<uint8_t>(fadeValue);
    }

    void setColor(const std::array<uint8_t, 3>& rgb)
    {
        for (size_t i = 0; i < 3; ++i)
            leds[i].setValue(rgb[i]);
    }
};
