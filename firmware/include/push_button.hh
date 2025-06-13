#pragma once

#include <Arduino.h>
#include <functional>

#include "hardware.hh"

class PushButton
{
    static constexpr auto LOG_TAG = "PushButton";
    gpio_num_t pin = static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Button::BUTTON1));

    unsigned long lastDown = 0;
    bool lastState = HIGH;
    bool longPressHandled = false;
    unsigned long longPressThreshold = 2000; // 2 seconds
    std::function<void()> longPressCallback;
    std::function<void()> shortPressCallback;

public:
    void setLongPressCallback(const std::function<void()>& callback)
    {
        longPressCallback = std::move(callback);
    }

    void setShortPressCallback(const std::function<void()>& callback)
    {
        shortPressCallback = std::move(callback);
    }

    explicit PushButton(const unsigned long longPressThreshold = 2000):
        longPressThreshold(longPressThreshold)
    {
        pinMode(this->pin, INPUT_PULLUP);
    }

    void handle(const unsigned long now)
    {
        const bool currentState = digitalRead(pin);

        if (lastState == HIGH && currentState == LOW)
        {
            // Button just pressed
            lastDown = now;
            longPressHandled = false;
        }
        else if (lastState == LOW && currentState == LOW)
        {
            // Button is being held
            if (!longPressHandled && (now - lastDown >= longPressThreshold))
            {
                if (longPressCallback) longPressCallback();
                longPressHandled = true;
            }
        }
        else if (lastState == LOW && currentState == HIGH)
        {
            // Button just released
            if (unsigned long pressDuration = now - lastDown; !longPressHandled && pressDuration < longPressThreshold)
            {
                if (shortPressCallback) shortPressCallback();
            }
        }

        lastState = currentState;
    }
};
