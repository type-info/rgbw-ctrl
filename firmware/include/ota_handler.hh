#pragma once

#include "ESPAsyncHTTPUpdateServer.h"

class OtaHandler
{
    ESPAsyncHTTPUpdateServer updateServer;

public:
    void begin(AsyncWebServer* webServer)
    {
        updateServer.setup(webServer);
    }
};
