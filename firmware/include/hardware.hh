#pragma once

#include <Arduino.h>
#include <array>
#include <optional>

namespace Hardware
{

    namespace Pin
    {

        enum class Input : uint8_t
        {
            VOLTAGE = GPIO_NUM_34
        };

        enum class Button : uint8_t
        {
            BUTTON1 = GPIO_NUM_0
        };

        enum class BoardLed : uint8_t
        {
            RED = GPIO_NUM_21,
            GREEN = GPIO_NUM_17,
            BLUE = GPIO_NUM_4
        };

        enum class Output : uint8_t
        {
            RED = GPIO_NUM_13,
            GREEN = GPIO_NUM_16,
            BLUE = GPIO_NUM_19,
            WHITE = GPIO_NUM_18
        };

        namespace Header
        {
            enum class H1 : uint8_t
            {
                P1 = GPIO_NUM_27,
                P2 = GPIO_NUM_26,
                P3 = GPIO_NUM_25,
                P4 = GPIO_NUM_33
            };

            enum class H2 : uint8_t
            {
                RX = GPIO_NUM_3,
                TX = GPIO_NUM_1
            };
        }

        static constexpr std::array<uint8_t, 5> INPUTS = {
            static_cast<uint8_t>(Button::BUTTON1),
            static_cast<uint8_t>(Header::H1::P1),
            static_cast<uint8_t>(Header::H1::P2),
            static_cast<uint8_t>(Header::H1::P3),
            static_cast<uint8_t>(Header::H1::P4)};

        static constexpr std::array<uint8_t, 4> OUTPUTS = {
            static_cast<uint8_t>(Output::RED),
            static_cast<uint8_t>(Output::GREEN),
            static_cast<uint8_t>(Output::BLUE),
            static_cast<uint8_t>(Output::WHITE)};

        static constexpr std::array<uint8_t, 3> LEDS = {
            static_cast<uint8_t>(BoardLed::RED),
            static_cast<uint8_t>(BoardLed::GREEN),
            static_cast<uint8_t>(BoardLed::BLUE)};
    }

    inline std::optional<uint8_t> getPwmChannel(const uint8_t pin)
    {
        switch (pin)
        {
            case static_cast<uint8_t>(Pin::BoardLed::RED): return 0;
            case static_cast<uint8_t>(Pin::BoardLed::GREEN): return 1;
            case static_cast<uint8_t>(Pin::BoardLed::BLUE): return 2;
            case static_cast<uint8_t>(Pin::Output::RED): return 3;
            case static_cast<uint8_t>(Pin::Output::GREEN): return 4;
            case static_cast<uint8_t>(Pin::Output::BLUE): return 5;
            case static_cast<uint8_t>(Pin::Output::WHITE): return 6;
            default: return std::nullopt;
        }
    }
}
