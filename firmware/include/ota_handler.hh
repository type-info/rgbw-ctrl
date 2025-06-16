#pragma once

#include <Update.h>
#include <StreamString.h>
#include <optional>
#include "webserver_handler.hh"

class OtaHandler
{
public:
    enum class UpdateState
    {
        Idle,
        Started,
        Completed,
        Failed
    };

    void begin(WebServerHandler& webServerHandler)
    {
        const auto handler = new AsyncOtaWebHandler(webServerHandler.getAuthenticationMiddleware());
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

        std::function<void(UpdateState state, uint8_t percentage)> onProgressCallback;
        const AsyncAuthenticationMiddleware& asyncAuthenticationMiddleware;

        mutable std::optional<String> updateError;
        mutable UpdateState updateState = UpdateState::Idle;
        mutable bool uploadCompleted = false;
        mutable size_t totalBytesExpected = 0;
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

            updateError.reset();
            totalBytesExpected = 0;
            totalBytesReceived = 0;

            if (request->hasHeader(CONTENT_LENGTH_HEADER))
                totalBytesExpected = request->header(CONTENT_LENGTH_HEADER).toInt();

            int name = U_FLASH;
            if (request->hasParam("name", false))
            {
                name = request->getParam("name")->value() == "filesystem" ? U_SPIFFS : U_FLASH;
            }
            if (Update.begin(totalBytesExpected == 0 ? UPDATE_SIZE_UNKNOWN : totalBytesExpected, name))
            {
                updateState = UpdateState::Started;
                uploadCompleted = false;
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
            {
                request->send(400, "text/plain", MSG_ALREADY_IN_PROGRESS);
                return;
            }
            if (updateError)
            {
                sendErrorResponse(request);
                if (updateState != UpdateState::Completed) Update.abort();
                resetUpdateState();
                return;
            }
            if (updateState != UpdateState::Started)
            {
                request->send(500, "text/plain", MSG_NO_SPACE);
                Update.abort();
                return;
            }
            if (!uploadCompleted)
            {
                request->send(500, "text/plain", MSG_UPLOAD_INCOMPLETE);
                return;
            }
            if (updateState == UpdateState::Completed)
            {
                request->send(200, "text/plain", MSG_ALREADY_FINALIZED);
                return;
            }

            if (Update.end(true))
            {
                updateState = UpdateState::Completed;
                ESP_LOGI(LOG_TAG, "Update successfully completed");
                request->send(200, "text/plain", MSG_SUCCESS);
                request->onDisconnect(restartAfterUpdate);
            }
            else
            {
                updateState = UpdateState::Failed;
                checkUpdateError();
                sendErrorResponse(request);
                resetUpdateState();
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
            if (!request->hasAttribute(ATTR_AUTHENTICATED)
                || request->hasAttribute(ATTR_DOUBLE_REQUEST))
                return;

            if (Update.write(data, len) != len)
            {
                updateState = UpdateState::Failed;
                checkUpdateError();
                return;
            }

            totalBytesReceived += len;
            reportProgress();

            if (!final) return;
            uploadCompleted = true;
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
            if (!request->hasAttribute(ATTR_AUTHENTICATED)
                || request->hasAttribute(ATTR_DOUBLE_REQUEST))
                return;

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
            request->send(500, "text/plain", updateError.value_or("Unknown OTA error"));
            updateError.reset();
        }

        void checkUpdateError() const
        {
            StreamString stream;
            Update.printError(stream);
            const char* error = stream.c_str();
            updateError = error;
            ESP_LOGE(LOG_TAG, "Update error: %s", error);
        }

        void resetUpdateState() const
        {
            updateState = UpdateState::Idle;
            uploadCompleted = false;
            totalBytesExpected = 0;
            totalBytesReceived = 0;
        }

        void reportProgress() const
        {
            if (onProgressCallback && totalBytesExpected > 0)
            {
                const auto percent = static_cast<uint8_t>((100 * totalBytesReceived) / totalBytesExpected);
                onProgressCallback(updateState, percent);
            }
        }

        static void restartAfterUpdate()
        {
            ESP_LOGI(LOG_TAG, "Restarting device after OTA update...");
            vTaskDelay(pdMS_TO_TICKS(100));
            ESP.restart();
        }

    public:
        explicit AsyncOtaWebHandler(const AsyncAuthenticationMiddleware& async_authentication_middleware)
            : asyncAuthenticationMiddleware(async_authentication_middleware)
        {
        }

        void setOnProgressCallback(std::function<void(UpdateState state, uint8_t percentage)> callback)
        {
            onProgressCallback = std::move(callback);
        }

        UpdateState getUpdateState() const
        {
            return updateState;
        }
    };

    AsyncOtaWebHandler* otaWebHandler = nullptr;
};
