#include <Arduino.h>

#include "wifi_manager.hh"
#include "board_led.hh"
#include "alexa_integration.hh"
#include "output.hh"
#include "push_button.hh"

Output output;
BoardLED boardLED;
OtaHandler otaHandler;
PushButton boardButton;
WiFiManager wifiManager;
AsyncWebServer webServer(80);
AsyncWebSocketMessageHandler wsHandler;
AlexaIntegration alexaIntegration(output);
AsyncWebSocket ws("/ws", wsHandler.eventHandler());
BleManager bleManager(wifiManager, alexaIntegration, otaHandler);

unsigned long lastClientConnectedAt = 0;

void setup()
{
    boardLED.begin();
    output.begin();
    wifiManager.begin();
    otaHandler.begin(&webServer);
    webServer.addHandler(&ws);
    // Can't call webServer.begin()
    // because alexaIntegration does it
    wifiManager.setGotIpCallback([]()
    {
        alexaIntegration.begin();
    });
    if (const auto credentials = WiFiManager::loadCredentials())
    {
        wifiManager.connect(credentials.value());
        lastClientConnectedAt = millis();
    }
    else
    {
        bleManager.begin();
    }

    boardButton.setLongPressCallback([]()
    {
        if (!bleManager.isInitialised())
            bleManager.begin();
    });

    boardButton.setShortPressCallback([]()
    {
        output.toggleAll();
    });
}

void loop()
{
    const unsigned long now = millis();

    boardButton.handle(now);
    alexaIntegration.handle();
    ws.cleanupClients();
    bleManager.handle();

    boardLED.handle(
        bleManager.isInitialised(),
        bleManager.isClientConnected(),
        wifiManager.getScanStatus(),
        wifiManager.getStatus()
    );

    if (bleManager.isClientConnected())
    {
        lastClientConnectedAt = now;
    }
    if (lastClientConnectedAt != 0 && now - lastClientConnectedAt > 5000)
    {
        // this will restart esp, since there's no way to restart ble
        bleManager.end();
    }
}
