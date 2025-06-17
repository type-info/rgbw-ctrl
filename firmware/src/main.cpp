#include <Arduino.h>
#include <nvs_flash.h>
#include <LittleFS.h>

#include "wifi_manager.hh"
#include "board_led.hh"
#include "alexa_integration.hh"
#include "output.hh"
#include "push_button.hh"
#include "ota_handler.hh"
#include "websocket_handler.hh"

Output output;
BoardLED boardLED;
OtaHandler otaHandler;
PushButton boardButton;
WiFiManager wifiManager;
WebServerHandler webServerHandler;
AlexaIntegration alexaIntegration(output);
BleManager bleManager(output,
                      wifiManager,
                      alexaIntegration,
                      webServerHandler);
WebSocketHandler webSocketHandler(output,
                                  otaHandler,
                                  wifiManager,
                                  webServerHandler,
                                  alexaIntegration,
                                  bleManager);

void setup()
{
    nvs_flash_init();
    boardLED.begin();
    output.begin(webServerHandler);
    wifiManager.begin();
    otaHandler.begin(webServerHandler, &bleManager);
    wifiManager.setGotIpCallback([]()
    {
        alexaIntegration.begin(webServerHandler);
        webServerHandler.begin(alexaIntegration.createAsyncWebHandler());
        webSocketHandler.begin(webServerHandler.getWebSocket());
    });

    if (const auto credentials = WiFiManager::loadCredentials())
        wifiManager.connect(credentials.value());
    else
        bleManager.start();

    boardButton.setLongPressCallback([]()
    {
        bleManager.start();
    });

    boardButton.setShortPressCallback([]()
    {
        output.toggleAll();
    });

    LittleFS.begin(true);
    webServerHandler.on("/restart", HTTP_GET, [](AsyncWebServerRequest* request)
    {
        request->onDisconnect([]()
        {
            bleManager.stop();
            esp_restart();
        });
        request->send(200, "text/plain", "Restarting...");
    });
    webServerHandler.on("/reset", HTTP_GET, [](AsyncWebServerRequest* request)
    {
        request->onDisconnect([]()
        {
            nvs_flash_erase();
            delay(300);
            bleManager.stop();
            esp_restart();
        });
        request->send(200, "text/plain", "Resetting to factory defaults...");
    });

    webServerHandler.on("/bluetooth", HTTP_GET, [](AsyncWebServerRequest* request)
    {
        auto state = request->getParam("state")->value() == "on";
        request->onDisconnect([state]()
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

void loop()
{
    const auto now = millis();

    boardButton.handle(now);
    alexaIntegration.handle();
    webServerHandler.handle();
    bleManager.handle(now);

    boardLED.handle(
        now,
        bleManager.isInitialised(),
        bleManager.isClientConnected(),
        wifiManager.getScanStatus(),
        wifiManager.getStatus(),
        otaHandler.getState() == OtaHandler::UpdateState::Started
    );
}
