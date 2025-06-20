#pragma once

#include "wifi_model.hh"
#include "ble_manager.hh"
#include "throttled_value.hh"

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

    AsyncWebSocket ws = AsyncWebSocket("/ws");

    ThrottledValue<std::array<LightState, 4>> outputThrottle{100};

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
        ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                          const AwsEventType type, void* arg, const uint8_t* data,
                          const size_t len)
        {
            this->handleWebSocketEvent(server, client, type, arg, data, len);
        });
    }

    void handle(const unsigned long now)
    {
        ws.cleanupClients();
        sendOutputColorMessage(now);
    }

    AsyncWebHandler* getAsyncWebHandler()
    {
        return &ws;
    }

private:
    void handleWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                              const AwsEventType type, void* arg, const uint8_t* data,
                              const size_t len)
    {
        switch (type)
        {
        case WS_EVT_CONNECT:
            ESP_LOGD(LOG_TAG, "WebSocket client connected: %s", client->remoteIP().toString().c_str());
            sendAllMessages(client);
            break;
        case WS_EVT_DISCONNECT:
            ESP_LOGD(LOG_TAG, "WebSocket client disconnected: %s", client->remoteIP().toString().c_str());
            break;
        case WS_EVT_PONG:
            ESP_LOGD(LOG_TAG, "WebSocket pong received from client: %s", client->remoteIP().toString().c_str());
            break;
        case WS_EVT_ERROR:
            ESP_LOGE(LOG_TAG, "WebSocket error: %s", client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DATA:
            this->handleWebSocketMessage(server, client, arg, data, len);
            break;
        default:
            break;
        }
    }

    void handleWebSocketMessage(
        AsyncWebSocket* server,
        AsyncWebSocketClient* client,
        void* arg,
        const uint8_t* data,
        const size_t len
    ) const
    {
        const auto info = static_cast<AwsFrameInfo*>(arg);
        if (info->opcode != WS_BINARY)
        {
            ESP_LOGD(LOG_TAG, "Received non-binary WebSocket message, opcode: %d", info->opcode);
            return;
        }
        if (!info->final)
        {
            ESP_LOGD(LOG_TAG, "Received fragmented WebSocket message, only final messages are processed");
            return;
        }
        if (info->index != 0)
        {
            ESP_LOGD(LOG_TAG, "Received fragmented WebSocket message with index %lld, only index 0 is processed",
                     info->index);
            return;
        }
        if (info->len != len)
        {
            ESP_LOGD(LOG_TAG, "Received WebSocket message with unexpected length: expected %lld, got %d", info->len,
                     len);
            return;
        }
        if (len < 1)
        {
            ESP_LOGD(LOG_TAG, "Received empty WebSocket message");
            return;
        }

        const uint8_t messageTypeRaw = data[0];
        if (messageTypeRaw > static_cast<uint8_t>(WebSocketMessageType::ON_ALEXA_INTEGRATION_SETTINGS))
        {
            ESP_LOGD(LOG_TAG, "Received unknown WebSocket message type: %d", messageTypeRaw);
            return;
        }

        const auto messageType = static_cast<WebSocketMessageType>(messageTypeRaw);
        ESP_LOGD(LOG_TAG, "Received WebSocket message of type %d", static_cast<int>(messageType));

        this->handleWebSocketMessage(messageType, server, client, data, len);
    }

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
        output.setState(message->values);
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
        case BleStatus::ADVERTISING:
            async_call([this]()
            {
                bleManager.start();
            }, 4096, 0);
            break;
        case BleStatus::OFF:
            bleManager.stop();
            break;
        default:
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

    void sendBleStatusMessage(const BleStatus status)
    {
        const BleStatusMessage message(status);
        const auto data = reinterpret_cast<const uint8_t*>(&message);
        ws.binaryAll(data, sizeof(message));
        ESP_LOGD(LOG_TAG, "Sent BLE status message: %u", static_cast<uint8_t>(status));
    }

    void sendAllMessages(AsyncWebSocketClient* client)
    {
        sendOutputColorMessage(millis(), client);
    }

    void sendOutputColorMessage(const unsigned long now, AsyncWebSocketClient* client = nullptr)
    {
        const auto state = output.getState();
        if (!outputThrottle.shouldSend(now, state) && !client)
            return;

        const ColorMessage message(state);
        const auto data = reinterpret_cast<const uint8_t*>(&message);
        constexpr size_t len = sizeof(message);

        if (client)
        {
            client->binary(data, len);
        }
        else if (AsyncWebSocket::SendStatus::ENQUEUED == ws.binaryAll(data, len))
        {
            outputThrottle.setLastSent(now, state);
        }
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
        const std::array<LightState, 4> values;

        explicit ColorMessage(const std::array<LightState, 4>& values)
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
