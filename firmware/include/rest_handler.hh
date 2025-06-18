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
    }
};
