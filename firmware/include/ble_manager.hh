#pragma once

#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <freertos/FreeRTOS.h>
#include <string>

#include "alexa_integration.hh"
#include "async_call.hh"
#include "version.hh"
#include "wifi_manager.hh"
#include "webserver_handler.hh"

enum class BleStatus :uint8_t
{
    OFF,
    ADVERTISING,
    CONNECTED
};

class BleManager
{
    struct BLE_UUID
    {
        static constexpr auto DEVICE_DETAILS_SERVICE = "12345678-1234-1234-1234-1234567890ac";
        static constexpr auto DEVICE_RESTART_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0000";
        static constexpr auto DEVICE_NAME_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0001";
        static constexpr auto FIRMWARE_VERSION_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0002";
        static constexpr auto HTTP_CREDENTIALS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0003";
        static constexpr auto DEVICE_HEAP_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0004";

        static constexpr auto WIFI_SERVICE = "12345678-1234-1234-1234-1234567890ab";
        static constexpr auto WIFI_DETAILS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0005";
        static constexpr auto WIFI_STATUS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0006";
        static constexpr auto WIFI_SCAN_STATUS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0007";
        static constexpr auto WIFI_SCAN_RESULT_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0008";

        static constexpr auto ALEXA_SERVICE = "12345678-1234-1234-1234-1234567890ba";
        static constexpr auto ALEXA_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0009";
        static constexpr auto ALEXA_COLOR_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee000a";
    };

    static constexpr auto LOG_TAG = "Network";
    static constexpr auto BLE_TIMEOUT_MS = 30000;

    unsigned long bluetoothTimeout = 0;

    Output& output;
    WiFiManager& wifiManager;
    AlexaIntegration& alexaIntegration;
    WebServerHandler& webServerHandler;

    NimBLEServer* server = nullptr;

    NimBLEService* deviceDetailsService = nullptr;
    NimBLECharacteristic* restartCharacteristic = nullptr;
    NimBLECharacteristic* deviceNameCharacteristic = nullptr;
    NimBLECharacteristic* firmwareVersionCharacteristic = nullptr;
    NimBLECharacteristic* httpCredentialsCharacteristic = nullptr;
    NimBLECharacteristic* deviceHeapCharacteristic = nullptr;

    NimBLEService* bleWiFiService = nullptr;
    NimBLECharacteristic* wifiDetailsCharacteristic = nullptr;
    NimBLECharacteristic* wifiStatusCharacteristic = nullptr;
    NimBLECharacteristic* wifiScanStatusCharacteristic = nullptr;
    NimBLECharacteristic* wifiScanResultCharacteristic = nullptr;

    NimBLEService* alexaService = nullptr;
    NimBLECharacteristic* alexaCharacteristic = nullptr;
    NimBLECharacteristic* alexaColorCharacteristic = nullptr;

    std::function<void(BleStatus)> bleStatusCallback;

public:
    explicit BleManager(Output& output, WiFiManager& wifiManager, AlexaIntegration& alexaIntegration,
                        WebServerHandler& webServerHandler)
        : output(output), wifiManager(wifiManager), alexaIntegration(alexaIntegration),
          webServerHandler(webServerHandler)
    {
    }

    void start()
    {
        bluetoothTimeout = millis() + BLE_TIMEOUT_MS;
        if (server != nullptr) return;
        wifiManager.setDetailsChangedCallback([this](WiFiDetails wiFiScanResult)
        {
            if (!wifiDetailsCharacteristic) return;
            wifiDetailsCharacteristic->setValue(reinterpret_cast<uint8_t*>(&wiFiScanResult), sizeof(wiFiScanResult));
            wifiDetailsCharacteristic->notify(); // NOLINT
        });

        wifiManager.setScanResultChangedCallback([this](WiFiScanResult wiFiScanResult)
        {
            if (!wifiScanResultCharacteristic) return;
            wifiScanResultCharacteristic->setValue(reinterpret_cast<uint8_t*>(&wiFiScanResult), sizeof(wiFiScanResult));
            wifiScanResultCharacteristic->notify(); // NOLINT
        });

        wifiManager.setScanStatusChangedCallback([this](WifiScanStatus wifiScanStatus)
        {
            if (!wifiScanStatusCharacteristic) return;
            wifiScanStatusCharacteristic->setValue(reinterpret_cast<uint8_t*>(&wifiScanStatus), sizeof(wifiScanStatus));
            wifiScanStatusCharacteristic->notify(); // NOLINT
        });

        wifiManager.setStatusChangedCallback([this](WiFiStatus wiFiScanResultStatus)
        {
            if (!wifiStatusCharacteristic) return;
            wifiStatusCharacteristic->setValue(reinterpret_cast<uint8_t*>(&wiFiScanResultStatus),
                                               sizeof(wiFiScanResultStatus));
            wifiStatusCharacteristic->notify(); // NOLINT
        });

        wifiManager.setDeviceNameChangedCallback([this](char* deviceName)
        {
            if (!deviceNameCharacteristic) return;
            const auto len = std::min(strlen(deviceName), static_cast<size_t>(DEVICE_NAME_MAX_LENGTH));
            deviceNameCharacteristic->setValue(reinterpret_cast<uint8_t*>(deviceName), len);
            deviceNameCharacteristic->notify(); // NOLINT
        });

        output.setNotifyBleCallback([this]()
        {
            if (!alexaColorCharacteristic) return;
            auto values = output.getValues();
            alexaColorCharacteristic->setValue(values.data(), values.size());
            alexaColorCharacteristic->notify(); // NOLINT
        });

        setupBle();
        const auto advertising = this->server->getAdvertising();
        advertising->setName(wifiManager.getDeviceName());
        advertising->start();
        if (bleStatusCallback) bleStatusCallback(BleStatus::ADVERTISING);
        ESP_LOGI(LOG_TAG, "BLE advertising started with device name: %s", wifiManager.getDeviceName());
    }

    void handle(const unsigned long now)
    {
        if (!deviceHeapCharacteristic) return;
        static auto lastSend = now;
        if (now - lastSend >= 500)
        {
            lastSend = now;
            auto heapSize = ESP.getFreeHeap();
            deviceHeapCharacteristic->setValue(reinterpret_cast<uint8_t*>(&heapSize), sizeof(heapSize));
            deviceHeapCharacteristic->notify(); // NOLINT
        }
        if (this->getStatus() == BleStatus::CONNECTED)
        {
            bluetoothTimeout = now + BLE_TIMEOUT_MS;
        }
        else if (now > bluetoothTimeout)
        {
            ESP_LOGW(LOG_TAG, "No BLE client connected for %d ms, stopping BLE server.", BLE_TIMEOUT_MS);
            this->stop();
        }
    }

    void stop() const
    {
        if (server == nullptr) return;
        server->getAdvertising()->stop();
        if (this->server->getConnectedCount() > 0)
        {
            server->disconnect(0); // NOLINT
            if (bleStatusCallback) bleStatusCallback(BleStatus::OFF);
            delay(100); // Allow client disconnect to propagate
        }
        esp_restart();
    }

    [[nodiscard]] BleStatus getStatus() const
    {
        if (this->server == nullptr)
            return BleStatus::OFF;
        if (this->server->getConnectedCount() > 0)
            return BleStatus::CONNECTED;
        return BleStatus::ADVERTISING;
    }

    [[nodiscard]] const char* getStatusString() const
    {
        switch (getStatus())
        {
        case BleStatus::OFF:
            return "OFF";
        case BleStatus::ADVERTISING:
            return "ADVERTISING";
        case BleStatus::CONNECTED:
            return "CONNECTED";
        default:
            return "UNKNOWN";
        }
    }

    void toJson(const JsonObject& to) const
    {
        to["status"] = getStatusString();
    }

    void setBleStatusCallback(const std::function<void(BleStatus)>& callback)
    {
        bleStatusCallback = callback;
        if (server != nullptr)
        {
            bleStatusCallback(getStatus());
        }
    }

private:
    void setupBle()
    {
        BLEDevice::init(wifiManager.getDeviceName());
        server = BLEDevice::createServer();
        server->setCallbacks(new BLEServerCallback(this));

        setupBleDeviceNameService(server);
        setupBleWiFiService(server);
        setupBleAlexaService(server);
    }

    void setupBleDeviceNameService(BLEServer* server)
    {
        deviceDetailsService = server->createService(BLE_UUID::DEVICE_DETAILS_SERVICE);

        restartCharacteristic = createCharacteristic(
            deviceDetailsService,
            BLE_UUID::DEVICE_RESTART_CHARACTERISTIC,
            WRITE,
            new RestartCallback(this)
        );

        deviceNameCharacteristic = createCharacteristic(
            deviceDetailsService,
            BLE_UUID::DEVICE_NAME_CHARACTERISTIC,
            WRITE | READ | NOTIFY,
            new DeviceNameCallback(this)
        );

        firmwareVersionCharacteristic = createCharacteristic(
            deviceDetailsService,
            BLE_UUID::FIRMWARE_VERSION_CHARACTERISTIC,
            READ,
            new FirmwareVersionCallback()
        );

        httpCredentialsCharacteristic = createCharacteristic(
            deviceDetailsService,
            BLE_UUID::HTTP_CREDENTIALS_CHARACTERISTIC,
            READ | WRITE,
            new HttpCredentialsCallback(this)
        );

        deviceHeapCharacteristic = createCharacteristic(
            deviceDetailsService,
            BLE_UUID::DEVICE_HEAP_CHARACTERISTIC,
            NOTIFY,
            nullptr
        );

        deviceDetailsService->start();
    }

    void setupBleWiFiService(BLEServer* server)
    {
        bleWiFiService = server->createService(BLE_UUID::WIFI_SERVICE);

        wifiDetailsCharacteristic = createCharacteristic(
            bleWiFiService,
            BLE_UUID::WIFI_DETAILS_CHARACTERISTIC,
            READ | NOTIFY,
            new WiFiDetailsCallback(this)
        );

        wifiStatusCharacteristic = createCharacteristic(
            bleWiFiService,
            BLE_UUID::WIFI_STATUS_CHARACTERISTIC,
            WRITE | READ | NOTIFY,
            new WiFiStatusCallback(this)
        );

        wifiScanStatusCharacteristic = createCharacteristic(
            bleWiFiService,
            BLE_UUID::WIFI_SCAN_STATUS_CHARACTERISTIC,
            WRITE | READ | NOTIFY,
            new WiFiScanStatusCallback(this)
        );

        wifiScanResultCharacteristic = createCharacteristic(
            bleWiFiService,
            BLE_UUID::WIFI_SCAN_RESULT_CHARACTERISTIC,
            READ | NOTIFY,
            new WiFiScanResultCallback(this)
        );

        bleWiFiService->start();
    }

    void setupBleAlexaService(BLEServer* server)
    {
        alexaService = server->createService(BLE_UUID::ALEXA_SERVICE);

        alexaCharacteristic = createCharacteristic(
            alexaService,
            BLE_UUID::ALEXA_CHARACTERISTIC,
            READ | WRITE,
            new AlexaCallback(this)
        );

        alexaColorCharacteristic = createCharacteristic(
            alexaService,
            BLE_UUID::ALEXA_COLOR_CHARACTERISTIC,
            READ | WRITE | NOTIFY,
            new AlexaColorCallback(this)
        );

        alexaService->start();
    }

    static NimBLECharacteristic* createCharacteristic(
        NimBLEService* service, const char* uuid, const uint32_t properties, NimBLECharacteristicCallbacks* cb)
    {
        const auto characteristic = service->createCharacteristic(uuid, properties);
        characteristic->setCallbacks(cb);
        return characteristic;
    }

    // -------------------- BLE CALLBACKS --------------------

    class RestartCallback final : public NimBLECharacteristicCallbacks
    {
    public:
        BleManager* net;

        explicit RestartCallback(BleManager* n) : net(n)
        {
        }

        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            if (pCharacteristic->getValue() == "RESTART_NOW")
            {
                ESP_LOGW(LOG_TAG, "Device restart requested via BLE.");
                async_call([this]()
                {
                    esp_restart();
                }, 1024, 50);
            }
            else
            {
                ESP_LOGW(LOG_TAG, "Device restart ignored: invalid value received.");
            }
        }
    };

    class DeviceNameCallback final : public NimBLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit DeviceNameCallback(BleManager* n) : net(n)
        {
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            const auto name = net->wifiManager.getDeviceName();
            const auto len = std::min(strlen(name), static_cast<size_t>(DEVICE_NAME_MAX_LENGTH));
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(const_cast<char*>(name)), len);
        }

        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            auto value = pCharacteristic->getValue();
            const auto data = value.data();
            const auto deviceName = reinterpret_cast<const char*>(data);

            if (const auto length = pCharacteristic->getLength(); length == 0 || length > DEVICE_NAME_MAX_LENGTH)
            {
                ESP_LOGE(LOG_TAG, "Invalid device name length: %d", static_cast<int>(length));
                return;
            }

            net->wifiManager.setDeviceName(deviceName);
        }
    };

    class FirmwareVersionCallback final : public NimBLECharacteristicCallbacks
    {
    public:
        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            pCharacteristic->setValue(FIRMWARE_VERSION);
        }
    };

    class HttpCredentialsCallback final : public NimBLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit HttpCredentialsCallback(BleManager* n) : net(n)
        {
        }

        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            HttpCredentials credentials;
            if (pCharacteristic->getValue().size() != sizeof(HttpCredentials))
            {
                ESP_LOGE(LOG_TAG, "Received invalid OTA credentials length: %d", pCharacteristic->getValue().size());
                return;
            }
            memcpy(&credentials, pCharacteristic->getValue().data(), sizeof(HttpCredentials));
            net->webServerHandler.updateCredentials(credentials);
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            HttpCredentials credentials = WebServerHandler::getCredentials();
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(&credentials), sizeof(credentials));
        }
    };

    class WiFiDetailsCallback final : public NimBLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit WiFiDetailsCallback(BleManager* n) : net(n)
        {
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            WiFiDetails details = WiFiDetails::fromWiFi();
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(&details), sizeof(details));
        }
    };

    class WiFiStatusCallback final : public NimBLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit WiFiStatusCallback(BleManager* n) : net(n)
        {
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            WiFiStatus status = net->wifiManager.getStatus();
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(&status), sizeof(status));
        }

        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            WiFiConnectionDetails details = {};
            if (pCharacteristic->getValue().size() != sizeof(WiFiConnectionDetails))
            {
                ESP_LOGE(LOG_TAG, "Received invalid WiFi connection details length: %d",
                         pCharacteristic->getValue().size());
                return;
            }
            memcpy(&details, pCharacteristic->getValue().data(), sizeof(WiFiConnectionDetails));
            net->wifiManager.connect(details);
        }
    };

    class WiFiScanStatusCallback final : public NimBLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit WiFiScanStatusCallback(BleManager* n) : net(n)
        {
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            WifiScanStatus scanStatus = net->wifiManager.getScanStatus();
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(&scanStatus), sizeof(scanStatus));
        }

        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            net->wifiManager.triggerScan();
        }
    };

    class WiFiScanResultCallback final : public NimBLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit WiFiScanResultCallback(BleManager* n) : net(n)
        {
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            WiFiScanResult scanResult = net->wifiManager.getScanResult();
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(&scanResult), sizeof(scanResult));
        }
    };

    class AlexaCallback final : public NimBLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit AlexaCallback(BleManager* net): net(net)
        {
        }

        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            AlexaIntegrationSettings settings;
            if (pCharacteristic->getValue().size() != sizeof(AlexaIntegrationSettings))
            {
                ESP_LOGE(LOG_TAG, "Received invalid Alexa settings length: %d", pCharacteristic->getValue().size());
                return;
            }
            memcpy(&settings, pCharacteristic->getValue().data(), sizeof(AlexaIntegrationSettings));
            net->alexaIntegration.applySettings(settings);
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            auto settings = net->alexaIntegration.getSettings();
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(&settings), sizeof(AlexaIntegrationSettings));
        }
    };

    class AlexaColorCallback final : public NimBLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit AlexaColorCallback(BleManager* net): net(net)
        {
        }

        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            std::array<uint8_t, 4> values = {};
            if (pCharacteristic->getValue().size() != values.size())
            {
                ESP_LOGE(LOG_TAG, "Received invalid Alexa color values length: %d", pCharacteristic->getValue().size());
                return;
            }
            memcpy(values.data(), pCharacteristic->getValue().data(), values.size());
            net->output.setValues(values, false);
            net->alexaIntegration.updateValues();
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            auto values = net->output.getValues();
            pCharacteristic->setValue(values.data(), values.size());
        }
    };

    class BLEServerCallback final : public NimBLEServerCallbacks
    {
        BleManager* net;

    public:
        explicit BLEServerCallback(BleManager* net): net(net)
        {
        }

        void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override
        {
            if (net->bleStatusCallback)
                net->bleStatusCallback(BleStatus::CONNECTED);
        }

        void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override
        {
            pServer->getAdvertising()->start();
            if (net->bleStatusCallback)
                net->bleStatusCallback(BleStatus::ADVERTISING);
        }
    };
};
