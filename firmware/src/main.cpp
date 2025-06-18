#include <Arduino.h>
#include <nvs_flash.h>
#include <LittleFS.h>

#include "wifi_manager.hh"
#include "board_led.hh"
#include "alexa_integration.hh"
#include "output.hh"
#include "push_button.hh"
#include "ota_handler.hh"
#include "rest_handler.hh"
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

RestHandler restHandler(output,
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
        restHandler.begin();
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
