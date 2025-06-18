#pragma once

#include <WiFi.h>
#include <cstring>

#include "ArduinoJson.h"

#define WIFI_MAX_SSID_LENGTH      32
#define WIFI_MAX_PASSWORD_LENGTH  64
#define WIFI_MAX_EAP_IDENTITY     128
#define WIFI_MAX_EAP_USERNAME     128
#define WIFI_MAX_EAP_PASSWORD     128
#define MAX_SCAN_NETWORK_COUNT    15

#pragma pack(push, 1)
enum class WifiScanEvent : uint8_t
{
    StartScan
};

enum class WifiScanStatus : uint8_t
{
    NOT_STARTED = 0,
    RUNNING = 1,
    COMPLETED = 2,
    FAILED = 3
};

enum class WiFiStatus : uint8_t
{
    DISCONNECTED = 0,
    CONNECTED = 1,
    CONNECTED_NO_IP = 2,
    WRONG_PASSWORD = 3,
    NO_AP_FOUND = 4,
    CONNECTION_FAILED = 5,
    UNKNOWN = 255
};

enum class WiFiEncryptionType : uint8_t
{
    OPEN = 0,
    WEP,
    WPA_PSK,
    WPA2_PSK,
    WPA_WPA2_PSK,
    ENTERPRISE,
    WPA2_ENTERPRISE = ENTERPRISE,
    WPA3_PSK,
    WPA2_WPA3_PSK,
    WAPI_PSK,
    WPA3_ENT_192,
    INVALID
};

enum class WiFiPhaseTwoType : uint8_t
{
    ESP_EAP_TTLS_PHASE2_EAP,
    ESP_EAP_TTLS_PHASE2_MSCHAPV2,
    ESP_EAP_TTLS_PHASE2_MSCHAP,
    ESP_EAP_TTLS_PHASE2_PAP,
    ESP_EAP_TTLS_PHASE2_CHAP
};

struct WiFiNetwork
{
    WiFiEncryptionType encryptionType = WiFiEncryptionType::INVALID;
    char ssid[WIFI_MAX_SSID_LENGTH + 1] = {};

    bool operator !=(const WiFiNetwork& other) const
    {
        return encryptionType != other.encryptionType || std::strcmp(ssid, other.ssid) != 0;
    }

    bool operator ==(const WiFiNetwork& other) const
    {
        return !(*this != other);
    }
};

struct WiFiScanResult
{
    uint8_t resultCount = 0;
    WiFiNetwork networks[MAX_SCAN_NETWORK_COUNT] = {};

    bool operator!=(const WiFiScanResult& other) const
    {
        if (resultCount != other.resultCount)
            return true;

        for (uint8_t i = 0; i < resultCount; ++i)
        {
            if (networks[i] != other.networks[i])
                return true;
        }

        return false;
    }

    [[nodiscard]] bool contains(const String& ssid) const
    {
        for (uint8_t i = 0; i < resultCount; ++i)
        {
            if (networks[i].ssid[0] == '\0')
                continue;
            if (String(networks[i].ssid) == ssid)
                return true;
        }
        return false;
    }
};

struct WiFiDetails
{
    char ssid[WIFI_MAX_SSID_LENGTH + 1];
    uint8_t mac[6];
    uint32_t ip;
    uint32_t gateway;
    uint32_t subnet;
    uint32_t dns;

    static WiFiDetails fromWiFi()
    {
        WiFiDetails details = {};
        WiFi.mode(WIFI_STA); // NOLINT
        std::memset(details.ssid, 0, sizeof(details.ssid));
        strncpy(details.ssid, WiFi.SSID().c_str(), WIFI_MAX_SSID_LENGTH);
        details.ssid[WIFI_MAX_SSID_LENGTH] = '\0';
        WiFi.macAddress(details.mac);
        details.ip = static_cast<uint32_t>(WiFi.localIP());
        details.gateway = static_cast<uint32_t>(WiFi.gatewayIP());
        details.subnet = static_cast<uint32_t>(WiFi.subnetMask());
        details.dns = static_cast<uint32_t>(WiFi.dnsIP());
        return details;
    }

    static void toJson(JsonObject to)
    {
        to["ssid"] = WiFi.SSID();
        to["mac"] = WiFi.macAddress();
        to["ip"] = WiFi.localIP().toString();
        to["gateway"] = WiFi.gatewayIP().toString();
        to["subnet"] = WiFi.subnetMask().toString();
        to["dns"] = WiFi.dnsIP().toString();
    }
};

struct WiFiConnectionDetails
{
    struct SimpleWiFiConnectionCredentials
    {
        char password[WIFI_MAX_PASSWORD_LENGTH + 1];
    };

    struct EAPWiFiConnectionCredentials
    {
        char password[WIFI_MAX_EAP_PASSWORD + 1];
        char identity[WIFI_MAX_EAP_IDENTITY + 1];
        char username[WIFI_MAX_EAP_USERNAME + 1];
        WiFiPhaseTwoType phase2Type;
    };

    union WiFiConnectionDetailsCredentials
    {
        SimpleWiFiConnectionCredentials simple;
        EAPWiFiConnectionCredentials eap;
    };

    WiFiEncryptionType encryptionType = WiFiEncryptionType::INVALID;
    char ssid[WIFI_MAX_SSID_LENGTH + 1] = {};
    WiFiConnectionDetailsCredentials credentials = {};
};

#pragma pack(pop)
