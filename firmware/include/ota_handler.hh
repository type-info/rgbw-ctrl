#pragma once

#include <Preferences.h>
#include <cstring>

#include "ESPAsyncHTTPUpdateServer.h"

struct OtaCredentials
{
    static constexpr auto MAX_USERNAME_LENGTH = 32;
    static constexpr auto MAX_PASSWORD_LENGTH = 32;

    char username[MAX_USERNAME_LENGTH + 1] = {};
    char password[MAX_PASSWORD_LENGTH + 1] = {};
};

class OtaHandler
{
    ESPAsyncHTTPUpdateServer updateServer;

public:
    void begin(AsyncWebServer* webServer)
    {
        auto [username, password] = getCredentials();
        updateServer.setup(webServer);
        updateServer.updateCredentials(username, password);
    }

    void updateCredentials(const OtaCredentials& credentials)
    {
        Preferences prefs;
        prefs.begin("ota-credentials", false);
        prefs.putString("username", credentials.username);
        prefs.putString("password", credentials.password);
        prefs.end();
        updateServer.updateCredentials(credentials.username, credentials.password);
    }

    [[nodiscard]] static OtaCredentials getCredentials()
    {
        OtaCredentials credentials;
        if (Preferences prefs; prefs.begin("ota-credentials", true))
        {
            strncpy(credentials.username, prefs.getString("username", "admin").c_str(),
                    OtaCredentials::MAX_USERNAME_LENGTH);
            strncpy(credentials.password, prefs.getString("password", "").c_str(), OtaCredentials::MAX_PASSWORD_LENGTH);
            prefs.end();
            return credentials;
        }
        Preferences prefs;
        prefs.begin("ota-credentials", false);
        strncpy(credentials.username, "admin", OtaCredentials::MAX_USERNAME_LENGTH);
        strncpy(credentials.password, generateRandomPassword().c_str(), OtaCredentials::MAX_PASSWORD_LENGTH);
        prefs.putString("username", credentials.username);
        prefs.putString("password", credentials.password);
        prefs.end();
        return credentials;
    }

private:
    [[nodiscard]] static String generateRandomPassword()
    {
        const auto v1 = random(100000, 999999);
        const auto v2 = random(100000, 999999);
        String password = String(v1) + "A-b" + String(v2);
        if (password.length() > OtaCredentials::MAX_PASSWORD_LENGTH)
        {
            password = password.substring(0, OtaCredentials::MAX_PASSWORD_LENGTH);
        }
        return password;
    }
};
