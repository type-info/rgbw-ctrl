#pragma once

#include <ArduinoJson.h>
#include <AsyncJson.h>

#include "version.hh"
#include "wifi_manager.hh"
#include "alexa_integration.hh"
#include "ble_manager.hh"
#include "ota_handler.hh"

class RestHandler
{
    Output& output;
    OtaHandler& otaHandler;
    WiFiManager& wifiManager;
    WebServerHandler& webServerHandler;
    AlexaIntegration& alexaIntegration;
    BleManager& bleManager;

public:
    RestHandler(
        Output& output,
        OtaHandler& otaHandler,
        WiFiManager& wifiManager,
        WebServerHandler& webServerHandler,
        AlexaIntegration& alexaIntegration,
        BleManager& bleManager
    )
        :
        output(output),
        otaHandler(otaHandler),
        wifiManager(wifiManager),
        webServerHandler(webServerHandler),
        alexaIntegration(alexaIntegration),
        bleManager(bleManager)
    {
    }

    void begin() const
    {
        webServerHandler.on("/rest/state", HTTP_GET, [this](AsyncWebServerRequest* request)
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
            otaHandler.toJson(doc["ota"].to<JsonObject>());

            response->addHeader("Cache-Control", "no-store");
            response->setLength();
            request->send(response);
        });

        webServerHandler.on("/rest/system/restart", HTTP_GET, [this](AsyncWebServerRequest* request)
        {
            request->onDisconnect([this]()
            {
                bleManager.stop();
                esp_restart();
            });
            request->send(200, "text/plain", "Restarting...");
        });

        webServerHandler.on("/rest/system/reset", HTTP_GET, [this](AsyncWebServerRequest* request)
        {
            request->onDisconnect([this]()
            {
                nvs_flash_erase();
                delay(300);
                bleManager.stop();
                esp_restart();
            });
            request->send(200, "text/plain", "Resetting to factory defaults...");
        });

        webServerHandler.on("/rest/bluetooth", HTTP_GET, [this](AsyncWebServerRequest* request)
        {
            auto state = request->getParam("state")->value() == "on";
            request->onDisconnect([this,state]()
            {
                if (state == true)
                    bleManager.start();
                else
                    bleManager.stop();
            });
            if (state)
                request->send(200, "text/plain", "Bluetooth enabled");
            else
                request->send(200, "text/plain", "Bluetooth disabled, device will restart");
        });

        webServerHandler.on("/rest/color", HTTP_GET, [this](AsyncWebServerRequest* request)
        {
            auto r = output.getValue(Color::Red);
            if (request->hasParam("r"))
            {
                r = std::max(std::min(request->getParam("r")->value().toInt(), 255l), 0l);
            }
            auto g = output.getValue(Color::Green);
            if (request->hasParam("g"))
            {
                g = std::max(std::min(request->getParam("g")->value().toInt(), 255l), 0l);
            }
            auto b = output.getValue(Color::Blue);
            if (request->hasParam("b"))
            {
                b = std::max(std::min(request->getParam("b")->value().toInt(), 255l), 0l);
            }
            auto w = output.getValue(Color::White);
            if (request->hasParam("w"))
            {
                w = std::max(std::min(request->getParam("w")->value().toInt(), 255l), 0l);
            }
            output.setColor(r, g, b, w);
            if (r > 0 || g > 0 || b > 0 || w > 0)
            {
                output.turnOn();
            }
            request->send(200, "text/plain", "Color set");
        });
    }
};
