#pragma once

#include <Update.h>
#include <optional>
#include <array>
#include "webserver_handler.hh"

class OtaHandler
{
public:
    static constexpr uint8_t MAX_UPDATE_ERROR_MSG_LEN = 64;

    enum class UpdateState
    {
        Idle,
        Started,
        Completed,
        Failed
    };

    void begin(WebServerHandler& webServerHandler, BleManager* bleManager)
    {
        const auto handler = new AsyncOtaWebHandler(webServerHandler.getAuthenticationMiddleware(), bleManager);
        webServerHandler.getWebServer()->addHandler(handler);
        otaWebHandler = handler;
    }

    void setOnProgressCallback(std::function<void(UpdateState state, uint8_t percentage)> callback) const
    {
        if (otaWebHandler)
            otaWebHandler->setOnProgressCallback(std::move(callback));
    }

    [[nodiscard]] UpdateState getState() const
    {
        if (otaWebHandler)
            return otaWebHandler->getUpdateState();
        return UpdateState::Idle;
    }

    [[nodiscard]] const char* getStateString() const
    {
        switch (getState())
        {
        case UpdateState::Idle: return "Idle";
        case UpdateState::Started: return "Update in progress";
        case UpdateState::Completed: return "Update completed successfully";
        case UpdateState::Failed: return "Update failed";
        }
        return "Unknown state";
    }

    void toJson(const JsonObject& to) const
    {
        to["state"] = getStateString();
    }

private:
    class AsyncOtaWebHandler final : public AsyncWebHandler
    {
        static constexpr auto REALM = "rgbw-ctrl";
        static constexpr auto LOG_TAG = "OtaHandler";
        static constexpr auto ATTR_DOUBLE_REQUEST = "double-request";
        static constexpr auto ATTR_AUTHENTICATED = "authenticated";
        static constexpr auto AUTHORIZATION_HEADER = "Authorization";
        static constexpr auto CONTENT_LENGTH_HEADER = "Content-Length";
        static constexpr auto MSG_NO_AUTH = "Authentication required for OTA update";
        static constexpr auto MSG_WRONG_CREDENTIALS = "Wrong credentials";
        static constexpr auto MSG_ALREADY_IN_PROGRESS = "OTA update already in progress";
        static constexpr auto MSG_NO_SPACE = "Not enough space for OTA update";
        static constexpr auto MSG_UPLOAD_INCOMPLETE = "OTA upload not completed";
        static constexpr auto MSG_ALREADY_FINALIZED = "OTA update already finalized";
        static constexpr auto MSG_SUCCESS = "OTA update successful";
        static constexpr auto MSG_BLUETOOTH_STARTED = "Bluetooth enabled, cannot perform OTA update";

        std::function<void(UpdateState state, uint8_t percentage)> onProgressCallback;
        const AsyncAuthenticationMiddleware& asyncAuthenticationMiddleware;
        BleManager* bleManager = nullptr;

        mutable std::optional<std::array<char, MAX_UPDATE_ERROR_MSG_LEN>> updateError;
        mutable std::atomic<UpdateState> updateState = UpdateState::Idle;
        mutable std::atomic<uint8_t> progressPercentage = 0;
        mutable bool uploadCompleted = false;
        mutable std::optional<size_t> totalBytesExpected = std::nullopt;
        mutable size_t totalBytesReceived = 0;

        bool canHandle(AsyncWebServerRequest* request) const override
        {
            if (request->method() != HTTP_POST || request->url() != "/update")
                return false;

            if (asyncAuthenticationMiddleware.allowed(request))
                request->setAttribute(ATTR_AUTHENTICATED, true);
            else
                return true;

            if (updateState == UpdateState::Started)
            {
                request->setAttribute(ATTR_DOUBLE_REQUEST, true);
                return true;
            }

            if (bleManager != nullptr && bleManager->getStatus() != BleStatus::OFF)
            {
                setUpdateError(MSG_BLUETOOTH_STARTED);
                return true;
            }

            resetUpdateState();
            updateState = UpdateState::Started;

            if (request->hasHeader(CONTENT_LENGTH_HEADER))
                totalBytesExpected = request->header(CONTENT_LENGTH_HEADER).toInt();

            request->onDisconnect([this]()
            {
                if (updateState != UpdateState::Completed)
                    Update.abort();
                else
                    restartAfterUpdate();
                resetUpdateState();
            });

            int updateTarget = U_FLASH;
            if (request->hasParam("name", false))
            {
                const String& nameParam = request->getParam("name")->value();
                updateTarget = nameParam == "filesystem" ? U_SPIFFS : U_FLASH;
            }

            if (Update.begin(totalBytesExpected.value_or(UPDATE_SIZE_UNKNOWN), updateTarget))
            {
                ESP_LOGI(LOG_TAG, "Update started");
            }
            else
            {
                updateState = UpdateState::Failed;
                checkUpdateError();
                ESP_LOGE(LOG_TAG, "Update.begin failed");
            }
            return true;
        }

        void handleRequest(AsyncWebServerRequest* request) override
        {
            if (!request->hasAttribute(ATTR_AUTHENTICATED))
            {
                const auto hasAuthorizationHeader = request->hasHeader(AUTHORIZATION_HEADER);
                const auto authFailMsg = hasAuthorizationHeader ? MSG_WRONG_CREDENTIALS : MSG_NO_AUTH;
                request->requestAuthentication(AsyncAuthType::AUTH_BASIC, REALM, authFailMsg);
                return;
            }
            if (request->hasAttribute(ATTR_DOUBLE_REQUEST))
                return request->send(400, "text/plain", MSG_ALREADY_IN_PROGRESS);

            if (updateError)
                return sendErrorResponse(request);

            if (updateState != UpdateState::Started)
                return request->send(500, "text/plain", MSG_NO_SPACE);

            if (!uploadCompleted)
            {
                ESP_LOGW(LOG_TAG, "OTA upload incomplete: received %u of %u bytes", totalBytesReceived,
                         totalBytesExpected);
                updateState = UpdateState::Idle;
                request->send(500, "text/plain", MSG_UPLOAD_INCOMPLETE);
                return;
            }
            if (updateState == UpdateState::Completed)
                return request->send(200, "text/plain", MSG_ALREADY_FINALIZED);

            if (Update.end(true))
            {
                updateState = UpdateState::Completed;
                ESP_LOGI(LOG_TAG, "Update successfully completed");
                request->send(200, "text/plain", MSG_SUCCESS);
            }
            else
            {
                updateState = UpdateState::Failed;
                checkUpdateError();
                sendErrorResponse(request);
            }
        }

        void handleUpload(
            AsyncWebServerRequest* request,
            const String& filename,
            const size_t index,
            uint8_t* data,
            const size_t len,
            const bool final
        ) override
        {
            if (updateState != UpdateState::Started) return;
            if (!isRequestValidForUpload(request)) return;

            if (Update.write(data, len) != len)
            {
                updateState = UpdateState::Failed;
                checkUpdateError();
                return;
            }

            totalBytesReceived += len;
            reportProgress();

            if (final) uploadCompleted = true;
        }

        void handleBody(
            AsyncWebServerRequest* request,
            uint8_t* data,
            const size_t len,
            const size_t index,
            const size_t total
        ) override
        {
            if (updateState != UpdateState::Started) return;
            if (!isRequestValidForUpload(request)) return;

            if (Update.write(data, len) != len)
            {
                updateState = UpdateState::Failed;
                checkUpdateError();
                return;
            }

            totalBytesReceived += len;
            reportProgress();

            if ((index + len) >= total)
                uploadCompleted = true;
        }

        void sendErrorResponse(AsyncWebServerRequest* request) const
        {
            request->send(500, "text/plain", updateError.has_value() ? updateError->data() : "Unknown OTA error");
            updateError.reset();
        }

        void checkUpdateError() const
        {
            const char* error = Update.errorString();
            setUpdateError(error);
            ESP_LOGE(LOG_TAG, "Update error: %s", error);
        }

        void setUpdateError(const char* error) const
        {
            std::array<char, MAX_UPDATE_ERROR_MSG_LEN> buffer = {};
            strncpy(buffer.data(), error, MAX_UPDATE_ERROR_MSG_LEN - 1);
            updateError = buffer;
        }

        void resetUpdateState() const
        {
            updateState = UpdateState::Idle;
            uploadCompleted = false;
            totalBytesExpected.reset();
            totalBytesReceived = 0;
            progressPercentage = 0;
        }

        void reportProgress() const
        {
            if (!totalBytesExpected.has_value()) return;
            progressPercentage = static_cast<uint8_t>((100 * totalBytesReceived) / totalBytesExpected.value());
            if (onProgressCallback)
            {
                onProgressCallback(updateState, progressPercentage);
            }
        }

        static void restartAfterUpdate()
        {
            ESP_LOGI(LOG_TAG, "Restarting device after OTA update...");
            vTaskDelay(pdMS_TO_TICKS(100));
            ESP.restart();
        }

        static bool isRequestValidForUpload(const AsyncWebServerRequest* request)
        {
            return request->hasAttribute(ATTR_AUTHENTICATED)
                && !request->hasAttribute(ATTR_DOUBLE_REQUEST);
        }

    public:
        explicit AsyncOtaWebHandler(const AsyncAuthenticationMiddleware& async_authentication_middleware,
                                    BleManager* bleManager)
            : asyncAuthenticationMiddleware(async_authentication_middleware), bleManager(bleManager)
        {
        }

        void setOnProgressCallback(std::function<void(UpdateState state, uint8_t percentage)> callback)
        {
            onProgressCallback = std::move(callback);
        }

        [[nodiscard]] UpdateState getUpdateState() const
        {
            return updateState;
        }

        [[nodiscard]] uint8_t getProgressPercentage() const
        {
            return progressPercentage;
        }
    };

    AsyncOtaWebHandler* otaWebHandler = nullptr;
};
