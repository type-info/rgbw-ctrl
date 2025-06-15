#pragma once

#include <WiFi.h>
#include <esp_wpa2.h>
#include <Preferences.h>
#include <cstring>
#include <atomic>
#include <mutex>

#include "wifi_model.hh"

#define DEVICE_NAME_MAX_LENGTH 28
#define DEVICE_BASE_NAME "rgbw-ctrl-"

class WiFiManager
{
    static constexpr auto LOG_TAG = "WiFiManager";
    static constexpr auto PREFERENCES_NAME = "wifi-config";

    std::atomic<WiFiStatus> wifiStatus = WiFiStatus::DISCONNECTED;
    std::atomic<WifiScanStatus> scanStatus = WifiScanStatus::COMPLETED;

    QueueHandle_t wifiScanQueue = nullptr;
    WiFiScanResult scanResult;

    std::function<void(WiFiDetails)> detailsChanged;
    std::function<void(WiFiScanResult)> scanResultChanged;
    std::function<void(WifiScanStatus)> scanStatusChanged;
    std::function<void(WiFiStatus)> statusChanged;
    std::function<void(char*)> deviceNameChanged;
    std::function<void()> gotIpChanged;

    char deviceName[DEVICE_NAME_MAX_LENGTH + 1] = {};

public:
    void begin()
    {
        WiFi.persistent(false);
        WiFi.mode(WIFI_MODE_STA);
        WiFi.onEvent([this](const WiFiEvent_t event, const WiFiEventInfo_t& info)
        {
            switch (event)
            {
            case ARDUINO_EVENT_WIFI_STA_CONNECTED:
                setStatus(WiFiStatus::CONNECTED_NO_IP);
                ESP_LOGI(LOG_TAG, "Connected to AP"); // NOLINT
                break;

            case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                ESP_LOGI(LOG_TAG, "Got IP: %s", WiFi.localIP().toString().c_str()); // NOLINT
                setStatus(WiFiStatus::CONNECTED);
                if (gotIpChanged) gotIpChanged();
                break;

            case ARDUINO_EVENT_WIFI_STA_LOST_IP:
                ESP_LOGW(LOG_TAG, "Lost IP address"); // NOLINT
                setStatus(WiFiStatus::CONNECTED_NO_IP);
                break;

            case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                ESP_LOGW(LOG_TAG, "Disconnected from AP. Reason: %d", info.wifi_sta_disconnected.reason); // NOLINT
                switch (info.wifi_sta_disconnected.reason)
                {
                case WIFI_REASON_AUTH_FAIL:
                    setStatus(WiFiStatus::WRONG_PASSWORD);
                    break;
                case WIFI_REASON_NO_AP_FOUND:
                    setStatus(WiFiStatus::NO_AP_FOUND);
                    break;
                default:
                    setStatus(WiFiStatus::DISCONNECTED);
                    break;
                }
                break;

            default:
                ESP_LOGD(LOG_TAG, "Unhandled WiFi event: %d", event);
                break;
            }
        });
        startTasks();
    }

    bool triggerScan() // NOLINT
    {
        constexpr auto event = WifiScanEvent::StartScan;
        if (const BaseType_t success = xQueueSend(wifiScanQueue, &event, 0); success == errQUEUE_FULL)
        {
            ESP_LOGW(LOG_TAG, "Scan request ignored: already in progress or queued");
            return false;
        }
        return true;
    }

    [[nodiscard]] WiFiStatus getStatus() const
    {
        return wifiStatus;
    }

    [[nodiscard]] WifiScanStatus getScanStatus() const
    {
        return scanStatus;
    }

    [[nodiscard]] WiFiScanResult getScanResult() const
    {
        std::lock_guard<std::mutex> lock(getWiFiScanResultMutex());
        return scanResult;
    }

    void setDetailsChangedCallback(std::function<void(WiFiDetails)> cb)
    {
        detailsChanged = std::move(cb);
    }

    void setScanResultChangedCallback(std::function<void(WiFiScanResult)> cb)
    {
        scanResultChanged = std::move(cb);
    }

    void setScanStatusChangedCallback(std::function<void(WifiScanStatus)> cb)
    {
        scanStatusChanged = std::move(cb);
    }

    void setStatusChangedCallback(std::function<void(WiFiStatus)> cb)
    {
        statusChanged = std::move(cb);
    }

    void setDeviceNameChangedCallback(std::function<void(char*)> cb)
    {
        deviceNameChanged = std::move(cb);
    }

    void setGotIpCallback(std::function<void()> cb)
    {
        gotIpChanged = std::move(cb);
    }

    const char* getDeviceName()
    {
        std::lock_guard<std::mutex> lock(getDeviceNameMutex());
        if (deviceName[0] == '\0')
        {
            loadDeviceName(deviceName);
        }
        return deviceName;
    }

    void setDeviceName(const char* name)
    {
        if (!name || name[0] == '\0') return;

        std::lock_guard<std::mutex> lock(getDeviceNameMutex());

        char safeName[DEVICE_NAME_MAX_LENGTH + 1];
        std::strncpy(safeName, name, DEVICE_NAME_MAX_LENGTH);
        safeName[DEVICE_NAME_MAX_LENGTH] = '\0';

        if (std::strncmp(deviceName, safeName, DEVICE_NAME_MAX_LENGTH) == 0)
            return;

        Preferences prefs;
        prefs.begin(PREFERENCES_NAME, false);
        prefs.putString("deviceName", safeName);
        prefs.end();

        deviceName[0] = '\0'; // Invalidate cached name
        WiFiClass::setHostname(safeName);
        WiFi.reconnect();

        if (deviceNameChanged)
        {
            deviceNameChanged(safeName);
        }
    }

    [[nodiscard]] static std::optional<WiFiConnectionDetails> loadCredentials()
    {
        WiFiConnectionDetails config = {};
        Preferences prefs;
        prefs.begin(PREFERENCES_NAME, true);
        config.encryptionType = static_cast<WiFiEncryptionType>(prefs.getUChar(
            "encryptionType", static_cast<uint8_t>(WiFiEncryptionType::INVALID)));
        if (config.encryptionType == WiFiEncryptionType::INVALID)
        {
            prefs.end();
            return std::nullopt; // No valid credentials found
        }
        if (isEap(config.encryptionType))
        {
            prefs.getBytes("identity", config.credentials.eap.identity, WIFI_MAX_EAP_IDENTITY + 1);
            prefs.getBytes("username", config.credentials.eap.username, WIFI_MAX_EAP_USERNAME + 1);
            prefs.getBytes("eapPassword", config.credentials.eap.password, WIFI_MAX_EAP_PASSWORD + 1);
            config.credentials.eap.phase2Type = static_cast<WiFiPhaseTwoType>(prefs.getUChar(
                "phase2Type", static_cast<uint8_t>(WiFiPhaseTwoType::ESP_EAP_TTLS_PHASE2_EAP)));
            prefs.getBytes("ssid", config.ssid, WIFI_MAX_SSID_LENGTH + 1);
        }
        else
        {
            prefs.getBytes("ssid", config.ssid, WIFI_MAX_SSID_LENGTH + 1);
            prefs.getBytes("password", config.credentials.simple.password, WIFI_MAX_PASSWORD_LENGTH + 1);
        }
        prefs.end();
        return config;
    }

    static void clearCredentials()
    {
        Preferences prefs;
        prefs.begin(PREFERENCES_NAME, false);

        prefs.remove("encryptionType");
        prefs.remove("ssid");
        prefs.remove("password");
        prefs.remove("identity");
        prefs.remove("username");
        prefs.remove("eapPassword");
        prefs.remove("phase2Type");

        prefs.end();
    }

    static bool isEap(const WiFiConnectionDetails& details)
    {
        return isEap(details.encryptionType);
    }

    static bool isEap(const WiFiEncryptionType& encryptionType)
    {
        switch (encryptionType)
        {
        case WiFiEncryptionType::WPA2_ENTERPRISE:
        case WiFiEncryptionType::WPA3_ENT_192:
            return true;
        default:
            return false;
        }
    }

    void connect(const WiFiConnectionDetails& details)
    {
        if (details.ssid[0] == '\0')
        {
            ESP_LOGE(LOG_TAG, "Cannot connect: SSID is empty");
            return;
        }
        saveCredentials(details);
        WiFiClass::setHostname(getDeviceName());

        if (const int result = WiFi.scanComplete(); result == WIFI_SCAN_RUNNING || result >= 0)
            WiFi.scanDelete();

        WiFi.mode(WIFI_STA); // NOLINT
        WiFi.disconnect(true);

        if (isEap(details))
            connect(details.ssid, details.credentials.eap);
        else
            connect(details.ssid, details.credentials.simple);
    }

private:
    static void saveCredentials(const WiFiConnectionDetails& details)
    {
        Preferences prefs;
        prefs.begin(PREFERENCES_NAME, false);
        prefs.putUChar("encryptionType", static_cast<uint8_t>(details.encryptionType));
        if (isEap(details))
        {
            prefs.putBytes("identity", details.credentials.eap.identity, WIFI_MAX_EAP_IDENTITY + 1);
            prefs.putBytes("username", details.credentials.eap.username, WIFI_MAX_EAP_USERNAME + 1);
            prefs.putBytes("eapPassword", details.credentials.eap.password, WIFI_MAX_EAP_PASSWORD + 1);
            prefs.putUChar("phase2Type", static_cast<uint8_t>(details.credentials.eap.phase2Type));
            prefs.putBytes("ssid", details.ssid, WIFI_MAX_SSID_LENGTH + 1);
            prefs.remove("password");
        }
        else
        {
            prefs.putBytes("ssid", details.ssid, WIFI_MAX_SSID_LENGTH + 1);
            prefs.putBytes("password", details.credentials.simple.password, WIFI_MAX_PASSWORD_LENGTH + 1);
            prefs.remove("identity");
            prefs.remove("username");
            prefs.remove("eapPassword");
            prefs.remove("phase2Type");
        }
        prefs.end();
    }

    void setStatus(WiFiStatus newStatus)
    {
        if (newStatus == wifiStatus) return;
        ESP_LOGI(LOG_TAG, "WiFi status changed: %d -> %d",
                 static_cast<int>(wifiStatus.load()), static_cast<int>(newStatus));
        wifiStatus = newStatus;
        if (statusChanged)
        {
            statusChanged(wifiStatus);
        }
        if (detailsChanged && (
            newStatus == WiFiStatus::CONNECTED
            || newStatus == WiFiStatus::DISCONNECTED
            || newStatus == WiFiStatus::CONNECTED_NO_IP
        ))
        {
            detailsChanged(WiFiDetails::fromWiFi());
        }
    }

    void setScanStatus(const WifiScanStatus status)
    {
        if (status == scanStatus) return;
        this->scanStatus = status;
        if (scanStatusChanged)
        {
            scanStatusChanged(status);
        }
    }

    void setScanResult(const WiFiScanResult& r)
    {
        std::lock_guard<std::mutex> lock(getWiFiScanResultMutex());
        if (r != scanResult)
        {
            scanResult = r;
            if (scanResultChanged)
            {
                scanResultChanged(r);
            }
        }
    }

    static std::mutex& getWiFiScanResultMutex()
    {
        static std::mutex mutex;
        return mutex;
    }

    static std::mutex& getDeviceNameMutex()
    {
        static std::mutex mutex;
        return mutex;
    }

    static void connect(const char* ssid, const WiFiConnectionDetails::SimpleWiFiConnectionCredentials& details)
    {
        esp_wifi_sta_wpa2_ent_disable();
        WiFi.begin(ssid, details.password[0] == '\0' ? nullptr : details.password);
    }

    static void connect(const char* ssid, const WiFiConnectionDetails::EAPWiFiConnectionCredentials& details)
    {
        esp_wifi_sta_wpa2_ent_enable();
        esp_wifi_sta_wpa2_ent_set_identity(reinterpret_cast<const unsigned char*>(details.identity),
                                           static_cast<int>(strlen(details.identity)));
        esp_wifi_sta_wpa2_ent_set_username(reinterpret_cast<const unsigned char*>(details.username),
                                           static_cast<int>(strlen(details.username)));
        esp_wifi_sta_wpa2_ent_set_password(reinterpret_cast<const unsigned char*>(details.password),
                                           static_cast<int>(strlen(details.password)));
        esp_wifi_sta_wpa2_ent_set_ttls_phase2_method(static_cast<esp_eap_ttls_phase2_types>(details.phase2Type));
        WiFi.begin(ssid);
    }

    void startTasks()
    {
        if (!wifiScanQueue)
        {
            wifiScanQueue = xQueueCreate(1, sizeof(WifiScanEvent));
            if (!wifiScanQueue)
            {
                ESP_LOGE(LOG_TAG, "Failed to create wifiScanQueue");
                return;
            }
            xTaskCreate(wifiScanNotifier, "WifiScanNotifier", 4096, this, 1, nullptr);
        }
    }

    static void wifiScanNotifier(void* param)
    {
        auto* manager = static_cast<WiFiManager*>(param);
        WifiScanEvent event;
        while (true) // NOLINT
        {
            if (xQueueReceive(manager->wifiScanQueue, &event, portMAX_DELAY) == pdTRUE
                && event == WifiScanEvent::StartScan)
            {
                manager->setScanStatus(WifiScanStatus::RUNNING);
                WiFi.scanNetworks(true);
                while (WiFi.scanComplete() == WIFI_SCAN_RUNNING)
                {
                    vTaskDelay(pdMS_TO_TICKS(500));
                }
                const int16_t scanStatus = WiFi.scanComplete();
                if (scanStatus < 0)
                {
                    ESP_LOGE(LOG_TAG, "WiFi scan failed with status: %d", scanStatus);
                    manager->setScanStatus(WifiScanStatus::FAILED);
                    continue;
                }
                WiFiScanResult result = {};

                for (int i = 0; i < scanStatus && result.resultCount < MAX_SCAN_NETWORK_COUNT; ++i)
                {
                    auto ssid = WiFi.SSID(i);
                    if (ssid.isEmpty()) continue;

                    if (result.contains(ssid))
                        continue; // Skip duplicates

                    strncpy(result.networks[result.resultCount].ssid, ssid.c_str(), WIFI_MAX_SSID_LENGTH);
                    result.networks[result.resultCount].ssid[WIFI_MAX_SSID_LENGTH] = '\0';
                    result.networks[result.resultCount].encryptionType =
                        static_cast<WiFiEncryptionType>(WiFi.encryptionType(i));
                    result.resultCount++;
                }
                WiFi.scanDelete();

                manager->setScanStatus(WifiScanStatus::COMPLETED);
                manager->setScanResult(result);
            }
        }
    } // NOLINT

    static const char* loadDeviceName(char* deviceName)
    {
        Preferences prefs;
        prefs.begin(PREFERENCES_NAME, true);
        if (prefs.isKey("deviceName"))
        {
            prefs.getString("deviceName", deviceName, DEVICE_NAME_MAX_LENGTH + 1);
            prefs.end();
            return deviceName;
        }
        prefs.end();
        uint8_t mac[6];
        WiFi.macAddress(mac);
        snprintf(deviceName, DEVICE_NAME_MAX_LENGTH + 1,
                 DEVICE_BASE_NAME "%02X%02X%02X", mac[3], mac[4], mac[5]);
        return deviceName;
    }
};
