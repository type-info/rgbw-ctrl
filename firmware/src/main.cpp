#include <Arduino.h>
#include <nvs_flash.h>
#include <LittleFS.h>

#include "wifi_manager.hh"
#include "board_led.hh"
#include "alexa_integration.hh"
#include "output.hh"
#include "push_button.hh"
#include "ota_handler.hh"

constexpr unsigned long BLE_TIMEOUT_MS = 15000;

Output output;
BoardLED boardLED;
OtaHandler otaHandler;
PushButton boardButton;
WiFiManager wifiManager;
WebServerHandler webServerHandler;
AlexaIntegration alexaIntegration(output);
BleManager bleManager(output, wifiManager, alexaIntegration, webServerHandler);

volatile unsigned long lastClientConnectedAt = 0;

void setup()
{
    nvs_flash_init();
    boardLED.begin();
    output.begin(webServerHandler);
    wifiManager.begin();
    otaHandler.begin(webServerHandler);
    wifiManager.setGotIpCallback([]()
    {
        alexaIntegration.begin(webServerHandler);
        webServerHandler.begin();
    });

    if (const auto credentials = WiFiManager::loadCredentials())
    {
        wifiManager.connect(credentials.value());
        lastClientConnectedAt = millis();
    }
    else
    {
        bleManager.start();
    }

    boardButton.setLongPressCallback([]()
    {
        lastClientConnectedAt = millis();
        bleManager.start();
    });

    boardButton.setShortPressCallback([]()
    {
        output.toggleAll();
    });

    LittleFS.begin(true);
    webServerHandler.on("/restart", HTTP_GET, [](AsyncWebServerRequest* request)
    {
        asyncCall([]()
        {
            bleManager.stop();
            esp_restart();
        }, 1024, 300);
        request->send(200, "text/plain", "Restarting...");
    });
    webServerHandler.on("/reset", HTTP_GET, [](AsyncWebServerRequest* request)
    {
        asyncCall([]()
        {
            nvs_flash_erase();
            delay(300);
            bleManager.stop();
            esp_restart();
        }, 2048, 300);
        request->send(200, "text/plain", "Resetting to factory defaults...");
    });

    webServerHandler.on("/bluetooth", HTTP_GET, [](AsyncWebServerRequest* request)
    {
        auto state = request->getParam("state")->value() == "on";
        asyncCall([state]()
        {
            if (state == true)
            {
                lastClientConnectedAt = millis();
                bleManager.start();
            }
            else
            {
                bleManager.stop();
            }
        }, 4096, 50);
        if (state)
            request->send(200, "text/plain", "Bluetooth enabled");
        else
            request->send(200, "text/plain", "Bluetooth disabled, device will restart");
    });
}

void loop()
{
    const auto now = millis();

    boardButton.handle(now);
    alexaIntegration.handle();
    webServerHandler.handle();
    bleManager.handle();

    boardLED.handle(
        now,
        bleManager.isInitialised(),
        bleManager.isClientConnected(),
        wifiManager.getScanStatus(),
        wifiManager.getStatus(),
        otaHandler.getState() == OtaHandler::UpdateState::Started
    );

    if (bleManager.isClientConnected())
    {
        lastClientConnectedAt = now;
    }
    if (lastClientConnectedAt != 0 && now - lastClientConnectedAt > BLE_TIMEOUT_MS)
    {
        // this will restart esp, since there's no way to restart ble
        bleManager.stop();
    }
}
