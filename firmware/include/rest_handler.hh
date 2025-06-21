#pragma once

#include <ArduinoJson.h>
#include <AsyncJson.h>

#include "version.hh"
#include "wifi_manager.hh"
#include "alexa_integration.hh"
#include "ble_manager.hh"
#include "ota_handler.hh"

enum class RestEndpoint
{
    State, Color, Bluetooth, Restart, Reset, Unknown
};

class RestHandler
{
    Output& output;
    OtaHandler& otaHandler;
    WiFiManager& wifiManager;
    AlexaIntegration& alexaIntegration;
    BleManager& bleManager;

public:
    RestHandler(
        Output& output,
        OtaHandler& otaHandler,
        WiFiManager& wifiManager,
        AlexaIntegration& alexaIntegration,
        BleManager& bleManager
    )
        :
        output(output),
        otaHandler(otaHandler),
        wifiManager(wifiManager),
        alexaIntegration(alexaIntegration),
        bleManager(bleManager)
    {
    }

    AsyncWebHandler* createAsyncWebHandler()
    {
        return new AsyncRestWebHandler(this);
    }

    void handleStateRequest(AsyncWebServerRequest* request) const
    {
        const auto response = new AsyncJsonResponse();
        const auto doc = response->getRoot().to<JsonObject>();
        doc["deviceName"] = wifiManager.getDeviceName();
        doc["firmwareVersion"] = FIRMWARE_VERSION;
        doc["heap"] = esp_get_free_heap_size();

        wifiManager.toJson(doc["wifi"].to<JsonObject>());
        alexaIntegration.getSettings().toJson(doc["alexa"].to<JsonObject>());
        output.toJson(doc["output"].to<JsonArray>());
        bleManager.toJson(doc["ble"].to<JsonObject>());
        otaHandler.getState().toJson(doc["ota"].to<JsonObject>());

        response->addHeader("Cache-Control", "no-store");
        response->setLength();
        request->send(response);
    }

    void handleRestartRequest(AsyncWebServerRequest* request) const
    {
        request->onDisconnect([this]()
        {
            bleManager.stop();
            esp_restart();
        });
        request->send(200, "text/plain", "Restarting...");
    }

    void handleResetRequest(AsyncWebServerRequest* request) const
    {
        request->onDisconnect([this]()
        {
            nvs_flash_erase();
            delay(300);
            bleManager.stop();
            esp_restart();
        });
        request->send(200, "text/plain", "Resetting to factory defaults...");
    }

    void handleBluetoothRequest(AsyncWebServerRequest* request) const
    {
        if (!request->hasParam("state"))
        {
            request->send(400, "text/plain", "Missing parameter: state");
            return;
        }

        auto state = request->getParam("state")->value() == "on";
        request->onDisconnect([this, state]()
        {
            if (state)
                bleManager.start();
            else
                bleManager.stop();
        });

        if (state)
            request->send(200, "text/plain", "Bluetooth enabled");
        else
            request->send(200, "text/plain", "Bluetooth disabled, device will restart");
    }

    void handleColorRequest(AsyncWebServerRequest* request) const
    {
        const auto r = extractParam(request, "r", Color::Red);
        const auto g = extractParam(request, "g", Color::Green);
        const auto b = extractParam(request, "b", Color::Blue);
        const auto w = extractParam(request, "w", Color::White);
        output.setColor(r, g, b, w);
        request->send(200, "text/plain", "Color set");
    }

    uint8_t extractParam(const AsyncWebServerRequest* req, const char* key, const Color color) const
    {
        if (req->hasParam(key))
            return std::clamp(static_cast<uint8_t>(req->getParam(key)->value().toInt()),
                              static_cast<uint8_t>(0), static_cast<uint8_t>(255));
        return output.getValue(color);
    }

    class AsyncRestWebHandler final : public AsyncWebHandler
    {
        RestHandler* restHandler;

    public:
        explicit AsyncRestWebHandler(RestHandler* restHandler): restHandler(restHandler)
        {
        }

    private:
        bool canHandle(AsyncWebServerRequest* request) const override
        {
            return request->url().startsWith("/rest");
        }

        void handleRequest(AsyncWebServerRequest* request) override
        {
            const auto path = request->url().substring(5);
            switch (getEndpoint(path))
            {
            case RestEndpoint::State:
                restHandler->handleStateRequest(request);
                break;
            case RestEndpoint::Color:
                restHandler->handleColorRequest(request);
                break;
            case RestEndpoint::Bluetooth:
                restHandler->handleBluetoothRequest(request);
                break;
            case RestEndpoint::Restart:
                restHandler->handleRestartRequest(request);
                break;
            case RestEndpoint::Reset:
                restHandler->handleResetRequest(request);
                break;
            default:
                request->send(404, "text/plain", "Not Found");
            }
        }

        static RestEndpoint getEndpoint(const String& path)
        {
            if (path == "/state") return RestEndpoint::State;
            if (path == "/color") return RestEndpoint::Color;
            if (path == "/bluetooth") return RestEndpoint::Bluetooth;
            if (path == "/system/restart") return RestEndpoint::Restart;
            if (path == "/system/reset") return RestEndpoint::Reset;
            return RestEndpoint::Unknown;
        }
    };
};
