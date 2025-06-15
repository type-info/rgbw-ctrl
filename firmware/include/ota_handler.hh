#pragma once

#include "ESPAsyncHTTPUpdateServer.h"
#include "http_credentials.hh"

struct HttpCredentials;

class OtaHandler
{
    ESPAsyncHTTPUpdateServer updateServer;

public:
    void begin(AsyncWebServer* webServer)
    {
        updateServer.setup(webServer);
    }

    void updateServerCredentials(const HttpCredentials& credentials)
    {
        updateServer.updateCredentials(credentials.username, credentials.password);
    }
};
