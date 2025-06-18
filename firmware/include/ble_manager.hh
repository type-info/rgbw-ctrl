#pragma once

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <freertos/FreeRTOS.h>
#include <cstring>
#include <string>

#include "alexa_integration.hh"
#include "AsyncJson.h"
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
    static constexpr auto BLE_TIMEOUT_MS = 15000;

    unsigned long bluetoothTimeout = 0;

    Output& output;
    WiFiManager& wifiManager;
    AlexaIntegration& alexaIntegration;
    WebServerHandler& webServerHandler;

    BLEServer* server = nullptr;

    BLEService* deviceDetailsService = nullptr;
    BLECharacteristic* restartCharacteristic = nullptr;
    BLECharacteristic* deviceNameCharacteristic = nullptr;
    BLECharacteristic* firmwareVersionCharacteristic = nullptr;
    BLECharacteristic* httpCredentialsCharacteristic = nullptr;
    BLECharacteristic* deviceHeapCharacteristic = nullptr;

    BLEService* bleWiFiService = nullptr;
    BLECharacteristic* wifiDetailsCharacteristic = nullptr;
    BLECharacteristic* wifiStatusCharacteristic = nullptr;
    BLECharacteristic* wifiScanStatusCharacteristic = nullptr;
    BLECharacteristic* wifiScanResultCharacteristic = nullptr;

    BLEService* alexaService = nullptr;
    BLECharacteristic* alexaCharacteristic = nullptr;
    BLECharacteristic* alexaColorCharacteristic = nullptr;

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
            wifiDetailsCharacteristic->notify();
        });

        wifiManager.setScanResultChangedCallback([this](WiFiScanResult wiFiScanResult)
        {
            if (!wifiScanResultCharacteristic) return;
            wifiScanResultCharacteristic->setValue(reinterpret_cast<uint8_t*>(&wiFiScanResult), sizeof(wiFiScanResult));
            wifiScanResultCharacteristic->notify();
        });

        wifiManager.setScanStatusChangedCallback([this](WifiScanStatus wifiScanStatus)
        {
            if (!wifiScanStatusCharacteristic) return;
            wifiScanStatusCharacteristic->setValue(reinterpret_cast<uint8_t*>(&wifiScanStatus), sizeof(wifiScanStatus));
            wifiScanStatusCharacteristic->notify();
        });

        wifiManager.setStatusChangedCallback([this](WiFiStatus wiFiScanResultStatus)
        {
            if (!wifiStatusCharacteristic) return;
            wifiStatusCharacteristic->setValue(reinterpret_cast<uint8_t*>(&wiFiScanResultStatus),
                                               sizeof(wiFiScanResultStatus));
            wifiStatusCharacteristic->notify();
        });

        wifiManager.setDeviceNameChangedCallback([this](char* deviceName)
        {
            if (!deviceNameCharacteristic) return;
            const auto len = std::min(strlen(deviceName), static_cast<size_t>(DEVICE_NAME_MAX_LENGTH));
            deviceNameCharacteristic->setValue(reinterpret_cast<uint8_t*>(deviceName), len);
            deviceNameCharacteristic->notify();
        });

        output.setOnChangeCallback([this](std::array<uint8_t, 4> output)
        {
            if (!alexaColorCharacteristic) return;
            alexaColorCharacteristic->setValue(output.data(), output.size());
            alexaColorCharacteristic->notify();
        });

        setupBle();
        this->server->getAdvertising()->start();
        ESP_LOGI(LOG_TAG, "BLE advertising started with device name: %s", wifiManager.getDeviceName());
    }

    void handle(const unsigned long now)
    {
        if (!deviceHeapCharacteristic) return;
        static auto lastSend = now;
        if (now - lastSend >= 1000)
        {
            lastSend = now;
            auto heapSize = ESP.getFreeHeap();
            deviceHeapCharacteristic->setValue(reinterpret_cast<uint8_t*>(&heapSize), sizeof(heapSize));
            deviceHeapCharacteristic->notify();
        }
        if (this->isClientConnected())
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
        if (this->isClientConnected())
        {
            server->disconnect(0);
            delay(100); // Allow client disconnect to propagate
        }
        esp_restart();
    }

    [[nodiscard]] bool isClientConnected() const
    {
        return this->server != nullptr && this->server->getConnectedCount() > 0;
    }

    [[nodiscard]] bool isInitialised() const
    {
        return this->server != nullptr;
    }

    [[nodiscard]] BleStatus getStatus() const
    {
        if (isClientConnected())
            return BleStatus::CONNECTED;
        if (isInitialised())
            return BleStatus::ADVERTISING;
        return BleStatus::OFF;
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

private:
    void setupBle()
    {
        BLEDevice::init(wifiManager.getDeviceName());
        server = BLEDevice::createServer();
        server->setCallbacks(new BLEServerCallback());

        setupBleDeviceNameService(server);
        setupBleWiFiService(server);
        setupBleAlexaService(server);

        server->getAdvertising()->setScanResponse(true);
    }

    void setupBleDeviceNameService(BLEServer* server)
    {
        deviceDetailsService = server->createService(BLE_UUID::DEVICE_DETAILS_SERVICE);

        restartCharacteristic = createCharacteristic(
            deviceDetailsService,
            BLE_UUID::DEVICE_RESTART_CHARACTERISTIC,
            BLECharacteristic::PROPERTY_WRITE,
            new RestartCallback(this)
        );

        deviceNameCharacteristic = createCharacteristic(
            deviceDetailsService,
            BLE_UUID::DEVICE_NAME_CHARACTERISTIC,
            BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY,
            new DeviceNameCallback(this)
        );

        firmwareVersionCharacteristic = createCharacteristic(
            deviceDetailsService,
            BLE_UUID::FIRMWARE_VERSION_CHARACTERISTIC,
            BLECharacteristic::PROPERTY_READ,
            new FirmwareVersionCallback()
        );

        httpCredentialsCharacteristic = createCharacteristic(
            deviceDetailsService,
            BLE_UUID::HTTP_CREDENTIALS_CHARACTERISTIC,
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE,
            new HttpCredentialsCallback(this)
        );

        deviceHeapCharacteristic = createCharacteristic(
            deviceDetailsService,
            BLE_UUID::DEVICE_HEAP_CHARACTERISTIC,
            BLECharacteristic::PROPERTY_NOTIFY,
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
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY,
            new WiFiDetailsCallback(this)
        );

        wifiStatusCharacteristic = createCharacteristic(
            bleWiFiService,
            BLE_UUID::WIFI_STATUS_CHARACTERISTIC,
            BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY,
            new WiFiStatusCallback(this)
        );

        wifiScanStatusCharacteristic = createCharacteristic(
            bleWiFiService,
            BLE_UUID::WIFI_SCAN_STATUS_CHARACTERISTIC,
            BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY,
            new WiFiScanStatusCallback(this)
        );

        wifiScanResultCharacteristic = createCharacteristic(
            bleWiFiService,
            BLE_UUID::WIFI_SCAN_RESULT_CHARACTERISTIC,
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY,
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
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE,
            new AlexaCallback(this)
        );

        alexaColorCharacteristic = createCharacteristic(
            alexaService,
            BLE_UUID::ALEXA_COLOR_CHARACTERISTIC,
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY,
            new AlexaColorCallback(this)
        );

        alexaService->start();
    }

    static BLECharacteristic* createCharacteristic(
        BLEService* service, const char* uuid, const uint32_t properties, BLECharacteristicCallbacks* cb)
    {
        const auto characteristic = service->createCharacteristic(uuid, properties);
        characteristic->setCallbacks(cb);
        if (properties & BLECharacteristic::PROPERTY_NOTIFY)
        {
            characteristic->addDescriptor(new BLE2902());
        }
        return characteristic;
    }

    // -------------------- BLE CALLBACKS --------------------

    class RestartCallback final : public BLECharacteristicCallbacks
    {
    public:
        BleManager* net;

        explicit RestartCallback(BleManager* n) : net(n)
        {
        }

        void onWrite(BLECharacteristic* c) override
        {
            if (c->getValue() == "RESTART_NOW")
            {
                ESP_LOGW(LOG_TAG, "Device restart requested via BLE.");
                net->server->disconnect(0);
                asyncCall([]()
                {
                    esp_restart();
                }, 1024, 100);
            }
            else
            {
                ESP_LOGW(LOG_TAG, "Device restart ignored: invalid value received.");
            }
        }
    };

    class DeviceNameCallback final : public BLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit DeviceNameCallback(BleManager* n) : net(n)
        {
        }

        void onRead(BLECharacteristic* c) override
        {
            const auto name = net->wifiManager.getDeviceName();
            const auto len = std::min(strlen(name), static_cast<size_t>(DEVICE_NAME_MAX_LENGTH));
            c->setValue(reinterpret_cast<uint8_t*>(const_cast<char*>(name)), len);
        }

        void onWrite(BLECharacteristic* pCharacteristic) override
        {
            const auto data = pCharacteristic->getData();
            const auto deviceName = reinterpret_cast<const char*>(data);

            if (const auto length = pCharacteristic->getLength(); length == 0 || length > DEVICE_NAME_MAX_LENGTH)
            {
                ESP_LOGE(LOG_TAG, "Invalid device name length: %d", static_cast<int>(length));
                return;
            }

            net->wifiManager.setDeviceName(deviceName);
        }
    };

    class FirmwareVersionCallback final : public BLECharacteristicCallbacks
    {
    public:
        void onRead(BLECharacteristic* pCharacteristic) override
        {
            pCharacteristic->setValue(FIRMWARE_VERSION);
        }
    };

    class HttpCredentialsCallback final : public BLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit HttpCredentialsCallback(BleManager* n) : net(n)
        {
        }

        void onWrite(BLECharacteristic* pCharacteristic) override
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

        void onRead(BLECharacteristic* pCharacteristic) override
        {
            HttpCredentials credentials = WebServerHandler::getCredentials();
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(&credentials), sizeof(credentials));
        }
    };

    class WiFiDetailsCallback final : public BLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit WiFiDetailsCallback(BleManager* n) : net(n)
        {
        }

        void onRead(BLECharacteristic* pCharacteristic) override
        {
            WiFiDetails details = WiFiDetails::fromWiFi();
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(&details), sizeof(details));
        }
    };

    class WiFiStatusCallback final : public BLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit WiFiStatusCallback(BleManager* n) : net(n)
        {
        }

        void onRead(BLECharacteristic* pCharacteristic) override
        {
            WiFiStatus status = net->wifiManager.getStatus();
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(&status), sizeof(status));
        }

        void onWrite(BLECharacteristic* pCharacteristic) override
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

    class WiFiScanStatusCallback final : public BLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit WiFiScanStatusCallback(BleManager* n) : net(n)
        {
        }

        void onRead(BLECharacteristic* pCharacteristic) override
        {
            WifiScanStatus scanStatus = net->wifiManager.getScanStatus();
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(&scanStatus), sizeof(scanStatus));
        }

        void onWrite(BLECharacteristic* pCharacteristic) override
        {
            net->wifiManager.triggerScan();
        }
    };

    class WiFiScanResultCallback final : public BLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit WiFiScanResultCallback(BleManager* n) : net(n)
        {
        }

        void onRead(BLECharacteristic* pCharacteristic) override
        {
            WiFiScanResult scanResult = net->wifiManager.getScanResult();
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(&scanResult), sizeof(scanResult));
        }
    };

    class AlexaCallback final : public BLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit AlexaCallback(BleManager* net): net(net)
        {
        }

        void onWrite(BLECharacteristic* pCharacteristic) override
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

        void onRead(BLECharacteristic* pCharacteristic) override
        {
            auto settings = net->alexaIntegration.getSettings();
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(&settings), sizeof(AlexaIntegrationSettings));
        }
    };

    class AlexaColorCallback final : public BLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit AlexaColorCallback(BleManager* net): net(net)
        {
        }

        void onWrite(BLECharacteristic* pCharacteristic) override
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

        void onRead(BLECharacteristic* pCharacteristic) override
        {
            auto values = net->output.getValues();
            pCharacteristic->setValue(values.data(), values.size());
        }
    };

    class BLEServerCallback final : public BLEServerCallbacks
    {
    public:
        void onDisconnect(BLEServer* pServer) override
        {
            pServer->getAdvertising()->start();
        }
    };
};
