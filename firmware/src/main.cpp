#include <Arduino.h>

#include "wifi_manager.hh"
#include "board_led.hh"
#include "alexa_integration.hh"
#include "output.hh"
#include "push_button.hh"

constexpr unsigned long BLE_TIMEOUT_MS = 5000;

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

void setupWebServer()
{
    webServer.addHandler(&ws);
    webServer.on("/bluetooth", HTTP_GET, [](AsyncWebServerRequest* request)
    {
        auto state = request->getParam("state")->value() == "on" ? true : false;
        asyncCall([state]()
        {
            if (state == true)
            {
                lastClientConnectedAt = 0;
                bleManager.start();
            }
            else
            {
                if (bleManager.isInitialised())
                    bleManager.stop();
            }
        }, 4096, 50);
        request->send(200, "text/plain", "Bluetooth enabled");
    });
    webServer.on("/restart", HTTP_GET, [](AsyncWebServerRequest* request)
    {
        asyncCall([]()
        {
            esp_restart();
        }, 1024, 300);
        request->send(200, "text/plain", "Restarting...");
    });
    webServer.on("/color", HTTP_GET, [](AsyncWebServerRequest* request)
    {
        if (request->hasParam("r") && request->hasParam("g") && request->hasParam("b"))
        {
            auto r = output.getValue(Color::Red);

            if (request->hasParam("r"))
            {
                r = request->getParam("r")->value().toInt();
            }
            auto g = output.getValue(Color::Green);
            if (request->hasParam("g"))
            {
                g = request->getParam("g")->value().toInt();
            }
            auto b = output.getValue(Color::Blue);
            if (request->hasParam("b"))
            {
                b = request->getParam("b")->value().toInt();
            }
            auto w = output.getValue(Color::White);
            if (request->hasParam("w"))
            {
                w = request->getParam("w")->value().toInt();
            }
            output.setColor(r, g, b, w);
        }
        request->send(200, "text/plain", "Color set");
    });
}

void setup()
{
    boardLED.begin();
    output.begin();
    wifiManager.begin();
    otaHandler.begin(&webServer);
    setupWebServer();
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
    if (lastClientConnectedAt != 0 && now - lastClientConnectedAt > BLE_TIMEOUT_MS)
    {
        // this will restart esp, since there's no way to restart ble
        bleManager.stop();
    }
}
