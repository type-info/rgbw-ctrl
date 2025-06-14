#include <Arduino.h>

#include "wifi_manager.hh"
#include "board_led.hh"
#include "alexa_integration.hh"
#include "output.hh"
#include "push_button.hh"
#include "ota_handler.hh"

constexpr unsigned long BLE_TIMEOUT_MS = 5000;

Output output;
BoardLED boardLED;
OtaHandler otaHandler;
PushButton boardButton;
WiFiManager wifiManager;
WebServerHandler webServerHandler;
AlexaIntegration alexaIntegration(output);
BleManager bleManager(output, wifiManager, alexaIntegration, webServerHandler);

unsigned long lastClientConnectedAt = 0;

void setup()
{
    boardLED.begin();
    output.begin(webServerHandler);
    wifiManager.begin();
    bleManager.begin();
    otaHandler.begin(webServerHandler.getWebServer());
    webServerHandler.begin();
    wifiManager.setGotIpCallback([]()
    {
        alexaIntegration.begin(webServerHandler);
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
        if (!bleManager.isInitialised())
            bleManager.stop();
    });

    boardButton.setShortPressCallback([]()
    {
        output.toggleAll();
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
        bleManager.isInitialised(),
        bleManager.isClientConnected(),
        wifiManager.getScanStatus(),
        wifiManager.getStatus()
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
