#pragma once

#include "wifi_model.hh"
#include "ble_manager.hh"

enum class WebSocketMessageType : uint8_t
{
    ON_COLOR,
    ON_HTTP_CREDENTIALS,
    ON_DEVICE_NAME,
    ON_HEAP,
    ON_BLE_STATUS,
    ON_WIFI_STATUS,
    ON_WIFI_SCAN_STATUS,
    ON_WIFI_DETAILS,
    ON_OTA_PROGRESS,
    ON_ALEXA_INTEGRATION_SETTINGS,
};

class WebSocketHandler
{
    static constexpr auto LOG_TAG = "WebSocketHandler";

    Output& output;
    OtaHandler& otaHandler;
    WiFiManager& wifiManager;
    WebServerHandler& webServerHandler;
    AlexaIntegration& alexaIntegration;
    BleManager& bleManager;

public:
    WebSocketHandler(
        Output& output,
        OtaHandler& otaHandler,
        WiFiManager& wifiManager,
        WebServerHandler& webServerHandler,
        AlexaIntegration& alexaIntegration,
        BleManager& bleManager
    )
        :
        output(output),
        otaHandler(otaHandler),
        wifiManager(wifiManager),
        webServerHandler(webServerHandler),
        alexaIntegration(alexaIntegration),
        bleManager(bleManager)
    {
    }

    void begin(AsyncWebSocketMessageHandler* webSocketHandler) const
    {
        webSocketHandler->onMessage(
            [this](AsyncWebSocket* server, AsyncWebSocketClient* client, const uint8_t* data, const size_t len)
            {
                if (len < 1)
                    return;

                const uint8_t messageTypeRaw = data[0];
                if (messageTypeRaw > static_cast<uint8_t>(WebSocketMessageType::ON_ALEXA_INTEGRATION_SETTINGS))
                {
                    ESP_LOGD(LOG_TAG, "Received unknown WebSocket message type: %d", messageTypeRaw);
                    return;
                }

                const auto messageType = static_cast<WebSocketMessageType>(messageTypeRaw);
                ESP_LOGD(LOG_TAG, "Received WebSocket message of type %d", static_cast<int>(messageType));

                this->handleWebSocketMessage(messageType, server, client, data, len);
            });
    }

private:
    void handleWebSocketMessage(
        const WebSocketMessageType messageType,
        AsyncWebSocket* server,
        AsyncWebSocketClient* client,
        const uint8_t* data,
        const size_t len
    ) const
    {
        switch (messageType)
        {
        case WebSocketMessageType::ON_COLOR:
            handleColorMessage(client, data, len);
            break;

        case WebSocketMessageType::ON_HTTP_CREDENTIALS:
            handleHttpCredentialsMessage(client, data, len);
            break;

        case WebSocketMessageType::ON_DEVICE_NAME:
            handleDeviceNameMessage(client, data, len);
            break;

        case WebSocketMessageType::ON_HEAP:
            handleHeapMessage(client);
            break;

        case WebSocketMessageType::ON_BLE_STATUS:
            handleBleStatusMessage(client, data, len);
            break;

        case WebSocketMessageType::ON_WIFI_STATUS:
            handleWiFiStatusMessage(client, data, len);
            break;

        case WebSocketMessageType::ON_WIFI_SCAN_STATUS:
            handleWiFiScanStatusMessage(client);
            break;

        case WebSocketMessageType::ON_WIFI_DETAILS:
            handleWiFiDetailsMessage(client, data, len);
            break;

        case WebSocketMessageType::ON_OTA_PROGRESS:
            handleOtaProgressMessage(client, data, len);
            break;

        case WebSocketMessageType::ON_ALEXA_INTEGRATION_SETTINGS:
            handleAlexaIntegrationSettingsMessage(client, data, len);
            break;

        default:
            client->text("Unknown message type");
            break;
        }
    }

    void handleColorMessage(AsyncWebSocketClient* client, const uint8_t* data, const size_t len) const
    {
        if (len < sizeof(ColorMessage)) return;
        const auto* message = reinterpret_cast<const ColorMessage*>(data);
        output.setValues(message->values);
    }

    void handleHttpCredentialsMessage(AsyncWebSocketClient* client, const uint8_t* data, const size_t len) const
    {
        if (len < sizeof(HttpCredentialsMessage)) return;
        const auto* message = reinterpret_cast<const HttpCredentialsMessage*>(data);
        webServerHandler.updateCredentials(message->credentials);
    }

    void handleDeviceNameMessage(AsyncWebSocketClient* client, const uint8_t* data, const size_t len) const
    {
        if (len < sizeof(DeviceNameMessage)) return;
        const auto* message = reinterpret_cast<const DeviceNameMessage*>(data);
        wifiManager.setDeviceName(message->deviceName);
    }

    static void handleHeapMessage(AsyncWebSocketClient* client)
    {
        ESP_LOGD(LOG_TAG, "Received HEAP message (ignored).");
    }

    void handleBleStatusMessage(AsyncWebSocketClient* client, const uint8_t* data, const size_t len) const
    {
        if (len < sizeof(BleStatusMessage)) return;
        switch (
            const auto* message = reinterpret_cast<const BleStatusMessage*>(data);
            message->status
        )
        {
        case BleStatus::ON:
            asyncCall([this]()
            {
                bleManager.start();
            }, 4096, 0);
            break;
        case BleStatus::OFF:
            bleManager.stop();
            break;
        }
    }

    void handleWiFiStatusMessage(AsyncWebSocketClient* client, const uint8_t* data, const size_t len) const
    {
        if (len < sizeof(WiFiConnectionDetailsMessage)) return;
        const auto* message = reinterpret_cast<const WiFiConnectionDetailsMessage*>(data);
        wifiManager.connect(message->details);
    }

    void handleWiFiScanStatusMessage(AsyncWebSocketClient* client) const
    {
        wifiManager.triggerScan();
    }

    static void handleWiFiDetailsMessage(AsyncWebSocketClient* client, const uint8_t* data, const size_t len)
    {
        ESP_LOGD(LOG_TAG, "Received WIFI_DETAILS message (ignored).");
    }

    static void handleOtaProgressMessage(AsyncWebSocketClient* client, const uint8_t* data, const size_t len)
    {
        ESP_LOGD(LOG_TAG, "Received OTA_PROGRESS message (ignored).");
    }

    void handleAlexaIntegrationSettingsMessage(AsyncWebSocketClient* client, const uint8_t* data,
                                               const size_t len) const
    {
        if (len < sizeof(AlexaIntegrationSettingsMessage)) return;
        const auto* message = reinterpret_cast<const AlexaIntegrationSettingsMessage*>(data);
        alexaIntegration.applySettings(message->settings);
    }

#pragma pack(push, 1)
    struct Message
    {
        WebSocketMessageType type;

        explicit Message(const WebSocketMessageType type) : type(type)
        {
        }
    };

    struct ColorMessage : Message
    {
        std::array<uint8_t, 4> values;

        explicit ColorMessage(const std::array<uint8_t, 4>& values)
            : Message(WebSocketMessageType::ON_COLOR), values(values)
        {
        }
    };

    struct HttpCredentialsMessage : Message
    {
        HttpCredentials credentials;

        explicit HttpCredentialsMessage(const HttpCredentials& credentials)
            : Message(WebSocketMessageType::ON_HTTP_CREDENTIALS), credentials(credentials)
        {
        }
    };

    struct DeviceNameMessage : Message
    {
        char deviceName[DEVICE_NAME_MAX_LENGTH + 1] = {};

        explicit DeviceNameMessage(const char* deviceName)
            : Message(WebSocketMessageType::ON_DEVICE_NAME)
        {
            std::strncpy(this->deviceName, deviceName, DEVICE_NAME_MAX_LENGTH);
            this->deviceName[DEVICE_NAME_MAX_LENGTH] = '\0';
        }
    };

    struct WiFiConnectionDetailsMessage : Message
    {
        WiFiConnectionDetails details;

        explicit WiFiConnectionDetailsMessage(const WiFiConnectionDetails& details)
            : Message(WebSocketMessageType::ON_WIFI_DETAILS), details(details)
        {
        }
    };

    struct BleStatusMessage : Message
    {
        BleStatus status;

        explicit BleStatusMessage(const BleStatus& status)
            : Message(WebSocketMessageType::ON_BLE_STATUS), status(status)
        {
        }
    };

    struct AlexaIntegrationSettingsMessage : Message
    {
        AlexaIntegrationSettings settings;

        explicit AlexaIntegrationSettingsMessage(const AlexaIntegrationSettings& settings)
            : Message(WebSocketMessageType::ON_ALEXA_INTEGRATION_SETTINGS), settings(settings)
        {
        }
    };

#pragma pack(pop)
};
