#pragma once

#include <cstring>
#include <utility>
#include <Espalexa.h>

#include "async_call.hh"
#include "output.hh"
#include "web_server.hh"

enum class AlexaIntegrationMode : uint8_t
{
    OFF = 0,
    RGBW_DEVICE = 1,
    RGB_DEVICE = 2,
    MULTI_DEVICE = 3
};

#pragma pack(push, 1)
struct AlexaIntegrationSettings
{
    static constexpr auto MAX_DEVICE_NAME_LENGTH = 32;

    AlexaIntegrationMode integrationMode = AlexaIntegrationMode::OFF;
    char rDeviceName[MAX_DEVICE_NAME_LENGTH] = "";
    char gDeviceName[MAX_DEVICE_NAME_LENGTH] = "";
    char bDeviceName[MAX_DEVICE_NAME_LENGTH] = "";
    char wDeviceName[MAX_DEVICE_NAME_LENGTH] = "";
};
#pragma pack(pop)

class AlexaIntegration
{
    Espalexa espalexa;
    Output& output;

public:
    explicit AlexaIntegration(Output& output): output(output)
    {
    }

    void begin()
    {
        webServer.onNotFound([this](AsyncWebServerRequest* request)
        {
            if (!espalexa.handleAlexaApiCall(request))
            {
                request->send(404, "text/plain", "Not found");
            }
        });
        const AlexaIntegrationSettings settings = loadPreferences();
        setupDevices(settings);
        espalexa.begin(&webServer);
    }

    void handle()
    {
        espalexa.loop();
    }

    static AlexaIntegrationSettings getSettings()
    {
        return loadPreferences();
    }

    static void applySettings(const AlexaIntegrationSettings& settings)
    {
        savePreferences(settings);
    }

private:
    [[nodiscard]] static AlexaIntegrationSettings loadPreferences()
    {
        AlexaIntegrationSettings settings;
        Preferences prefs;
        prefs.begin("alexa-config", true);

        const auto mode = prefs.getUChar("mode", static_cast<uint8_t>(AlexaIntegrationMode::OFF));
        if (mode <= static_cast<uint8_t>(AlexaIntegrationMode::MULTI_DEVICE))
        {
            settings.integrationMode = static_cast<AlexaIntegrationMode>(mode);
        }
        else
        {
            settings.integrationMode = AlexaIntegrationMode::OFF;
        }

        const String r = prefs.getString("r", "");
        const String g = prefs.getString("g", "");
        const String b = prefs.getString("b", "");
        const String w = prefs.getString("w", "");
        prefs.end();

        strncpy(settings.rDeviceName, r.c_str(), AlexaIntegrationSettings::MAX_DEVICE_NAME_LENGTH - 1);
        strncpy(settings.gDeviceName, g.c_str(), AlexaIntegrationSettings::MAX_DEVICE_NAME_LENGTH - 1);
        strncpy(settings.bDeviceName, b.c_str(), AlexaIntegrationSettings::MAX_DEVICE_NAME_LENGTH - 1);
        strncpy(settings.wDeviceName, w.c_str(), AlexaIntegrationSettings::MAX_DEVICE_NAME_LENGTH - 1);

        settings.rDeviceName[AlexaIntegrationSettings::MAX_DEVICE_NAME_LENGTH - 1] = '\0';
        settings.gDeviceName[AlexaIntegrationSettings::MAX_DEVICE_NAME_LENGTH - 1] = '\0';
        settings.bDeviceName[AlexaIntegrationSettings::MAX_DEVICE_NAME_LENGTH - 1] = '\0';
        settings.wDeviceName[AlexaIntegrationSettings::MAX_DEVICE_NAME_LENGTH - 1] = '\0';
        return settings;
    }

    static void savePreferences(AlexaIntegrationSettings settings)
    {
        Preferences prefs;
        prefs.begin("alexa-config", false);
        prefs.putUChar("mode", static_cast<uint8_t>(settings.integrationMode));
        prefs.putString("r", settings.rDeviceName);
        prefs.putString("g", settings.gDeviceName);
        prefs.putString("b", settings.bDeviceName);
        prefs.putString("w", settings.wDeviceName);
        prefs.end();
    }

    void setupDevices(const AlexaIntegrationSettings& settings)
    {
        switch (settings.integrationMode)
        {
        case AlexaIntegrationMode::OFF:
            break;
        case AlexaIntegrationMode::RGBW_DEVICE:
            setupRgbwDevice(settings);
            break;
        case AlexaIntegrationMode::RGB_DEVICE:
            setupRgbDevice(settings);
            break;
        case AlexaIntegrationMode::MULTI_DEVICE:
            setupMultiDevice(settings);
            break;
        }
    }

    void setupRgbwDevice(const AlexaIntegrationSettings& settings)
    {
        if (settings.rDeviceName[0] == '\0') return;
        ESP_LOGI("AlexaIntegration", "Adding RGBW device: %s", settings.rDeviceName);
        espalexa.addDevice(settings.rDeviceName, [this](const uint8_t brightness, const uint32_t color)
        {
            uint8_t r = (color >> 16) & 0xFF;
            uint8_t g = (color >> 8) & 0xFF;
            uint8_t b = color & 0xFF;
            const float intensity = static_cast<float>(brightness) / 255.0f;
            r = static_cast<uint8_t>(static_cast<float>(r) * intensity);
            g = static_cast<uint8_t>(static_cast<float>(g) * intensity);
            b = static_cast<uint8_t>(static_cast<float>(b) * intensity);
            const uint8_t w = std::min({r, g, b});
            r -= w;
            g -= w;
            b -= w;
            output.setColor(r, g, b, w);
        }, 0);
    }

    void setupRgbDevice(const AlexaIntegrationSettings& settings)
    {
        if (settings.rDeviceName[0] != '\0')
        {
            ESP_LOGI("AlexaIntegration", "Adding RGB device: %s", settings.rDeviceName);
            espalexa.addDevice(settings.rDeviceName, [&](const uint8_t brightness, const uint32_t color)
            {
                ESP_LOGI("AlexaIntegration", "Received %s command: brightness=%d, color=0x%06X", settings.rDeviceName,
                         brightness, color);
                uint8_t r = (color >> 16) & 0xFF;
                uint8_t g = (color >> 8) & 0xFF;
                uint8_t b = color & 0xFF;
                const float intensity = static_cast<float>(brightness) / 255.0f;
                r = static_cast<uint8_t>(static_cast<float>(r) * intensity);
                g = static_cast<uint8_t>(static_cast<float>(g) * intensity);
                b = static_cast<uint8_t>(static_cast<float>(b) * intensity);
                output.update(Color::Red, r);
                output.update(Color::Green, g);
                output.update(Color::Blue, b);
            }, 0);
        }

        if (settings.wDeviceName[0] != '\0')
        {
            addSingleChannelDevice(settings.wDeviceName, Color::White);
        }
    }

    void setupMultiDevice(const AlexaIntegrationSettings& settings)
    {
        addSingleChannelDevice(settings.rDeviceName, Color::Red);
        addSingleChannelDevice(settings.gDeviceName, Color::Green);
        addSingleChannelDevice(settings.bDeviceName, Color::Blue);
        addSingleChannelDevice(settings.wDeviceName, Color::White);
    }

    void addSingleChannelDevice(const char* name, Color color)
    {
        if (name[0] != '\0')
        {
            ESP_LOGI("AlexaIntegration", "Adding device: %s", name);
            espalexa.addDevice(
                name,
                [this, name, color](const uint8_t brightness)
                {
                    ESP_LOGI("AlexaIntegration", "Received %s command: brightness=%d", name, brightness);
                    output.update(color, brightness);
                },
                0
            );
        }
    }
};
