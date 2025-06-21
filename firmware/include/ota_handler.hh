#pragma once

#include <Update.h>
#include <optional>
#include <array>
#include <atomic>
#include "webserver_handler.hh"

enum class OtaStatus : uint8_t
{
    Idle,
    Started,
    Completed,
    Failed
};

#pragma pack(push, 1)
struct OtaState
{
    OtaStatus status = OtaStatus::Idle;
    uint32_t totalBytesExpected = 0;
    uint32_t totalBytesReceived = 0;

    void toJson(const JsonObject& to) const
    {
        to["status"] = statusToString(status);
        to["totalBytesExpected"] = totalBytesExpected;
        to["totalBytesReceived"] = totalBytesReceived;
    }

    [[nodiscard]] static const char* statusToString(const OtaStatus status)
    {
        switch (status)
        {
        case OtaStatus::Idle: return "Idle";
        case OtaStatus::Started: return "Update in progress";
        case OtaStatus::Completed: return "Update completed successfully";
        case OtaStatus::Failed: return "Update failed";
        }
        return "Unknown state";
    }

    bool operator==(const OtaState& other) const
    {
        return this->status == other.status &&
            this->totalBytesExpected == other.totalBytesExpected &&
            this->totalBytesReceived == other.totalBytesReceived;
    }

    bool operator!=(const OtaState& other) const
    {
        return !(*this == other);
    }
};
#pragma pack(pop)

class OtaHandler
{
public:
    static constexpr uint8_t MAX_UPDATE_ERROR_MSG_LEN = 64;

    void begin(WebServerHandler& webServerHandler)
    {
        const auto handler = new AsyncOtaWebHandler(webServerHandler.getAuthenticationMiddleware());
        webServerHandler.getWebServer()->addHandler(handler);
        otaWebHandler = handler;
    }

    [[nodiscard]] OtaState getState() const
    {
        return otaWebHandler ? otaWebHandler->getState() : OtaState{};
    }

    [[nodiscard]] OtaStatus getStatus() const
    {
        return otaWebHandler ? otaWebHandler->getStatus() : OtaStatus::Idle;
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

        const AsyncAuthenticationMiddleware& asyncAuthenticationMiddleware;

        mutable std::optional<std::array<char, MAX_UPDATE_ERROR_MSG_LEN>> updateError;
        mutable bool uploadCompleted = false;

        // `status` is atomic because it's read and written from multiple handlers concurrently.
        mutable std::atomic<OtaStatus> status = OtaStatus::Idle;

        // `totalBytesExpected/Received` are volatile for visibility during upload monitoring only.
        mutable volatile uint32_t totalBytesExpected = 0;
        mutable volatile uint32_t totalBytesReceived = 0;

        bool canHandle(AsyncWebServerRequest* request) const override
        {
            if (request->url() != "/update")
                return false;

            if (request->method() != HTTP_POST && request->method() != HTTP_GET)
                return false;

            if (asyncAuthenticationMiddleware.allowed(request))
                request->setAttribute(ATTR_AUTHENTICATED, true);
            else
                return true;

            if (status == OtaStatus::Started)
            {
                request->setAttribute(ATTR_DOUBLE_REQUEST, true);
                return true;
            }

            resetUpdateState();
            status = OtaStatus::Started;

            if (request->hasHeader(CONTENT_LENGTH_HEADER))
                totalBytesExpected = request->header(CONTENT_LENGTH_HEADER).toInt();

            if (request->hasParam("md5", false))
            {
                const String& md5Param = request->getParam("md5")->value();
                if (!Update.setMD5(md5Param.c_str()))
                {
                    setUpdateError("Invalid MD5 format");
                    status = OtaStatus::Failed;
                    return true;
                }
            }

            request->onDisconnect([this]
            {
                if (status != OtaStatus::Completed)
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

            if (const unsigned int expected = totalBytesExpected;
                Update.begin(expected == 0 ? UPDATE_SIZE_UNKNOWN : expected, updateTarget))
            {
                ESP_LOGI(LOG_TAG, "Update started");
            }
            else
            {
                status = OtaStatus::Failed;
                checkUpdateError();
                ESP_LOGE(LOG_TAG, "Update.begin failed");
            }
            return true;
        }

        void handleRequest(AsyncWebServerRequest* request) override
        {
            if (request->method() == HTTP_GET)
            {
                request->redirect("/ota.xml");
                return;
            }
            if (!request->hasAttribute(ATTR_AUTHENTICATED))
            {
                const auto hasAuthorizationHeader = request->hasHeader(AUTHORIZATION_HEADER);
                const auto authFailMsg = hasAuthorizationHeader ? MSG_WRONG_CREDENTIALS : MSG_NO_AUTH;
                request->requestAuthentication(AUTH_BASIC, REALM, authFailMsg);
                return;
            }
            if (request->hasAttribute(ATTR_DOUBLE_REQUEST))
                return request->send(400, "text/plain", MSG_ALREADY_IN_PROGRESS);

            if (updateError)
                return sendErrorResponse(request);

            if (status != OtaStatus::Started)
                return request->send(500, "text/plain", MSG_NO_SPACE);

            if (!uploadCompleted)
            {
                ESP_LOGW(LOG_TAG, "OTA upload incomplete: received %u of %u bytes",
                         totalBytesReceived, totalBytesExpected);
                status = OtaStatus::Idle;
                request->send(500, "text/plain", MSG_UPLOAD_INCOMPLETE);
                return;
            }
            if (status == OtaStatus::Completed)
                return request->send(200, "text/plain", MSG_ALREADY_FINALIZED);

            if (Update.end(true))
            {
                status = OtaStatus::Completed;
                ESP_LOGI(LOG_TAG, "Update successfully completed");
                request->send(200, "text/plain", MSG_SUCCESS);
            }
            else
            {
                status = OtaStatus::Failed;
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
            if (status != OtaStatus::Started) return;
            if (!isRequestValidForUpload(request)) return;

            if (Update.write(data, len) != len)
            {
                status = OtaStatus::Failed;
                checkUpdateError();
                return;
            }

            totalBytesReceived += len;

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
            if (status != OtaStatus::Started) return;
            if (!isRequestValidForUpload(request)) return;

            if (Update.write(data, len) != len)
            {
                status = OtaStatus::Failed;
                checkUpdateError();
                return;
            }

            totalBytesReceived += len;

            if (index + len >= total)
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
        }

        void setUpdateError(const char* error) const
        {
            std::array<char, MAX_UPDATE_ERROR_MSG_LEN> buffer = {};
            strncpy(buffer.data(), error, MAX_UPDATE_ERROR_MSG_LEN - 1);
            updateError = buffer;
            ESP_LOGE(LOG_TAG, "Update error: %s", error);
        }

        void resetUpdateState() const
        {
            status = OtaStatus::Idle;
            uploadCompleted = false;
            totalBytesExpected = 0;
            totalBytesReceived = 0;
            updateError.reset();
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
        explicit AsyncOtaWebHandler(const AsyncAuthenticationMiddleware& asyncAuthenticationMiddleware)
            : asyncAuthenticationMiddleware(asyncAuthenticationMiddleware)
        {
        }

        [[nodiscard]] OtaState getState() const
        {
            return {
                status.load(std::memory_order_relaxed),
                totalBytesExpected,
                totalBytesReceived
            };
        }

        [[nodiscard]] OtaStatus getStatus() const
        {
            return status.load(std::memory_order_relaxed);
        }
    };

    AsyncOtaWebHandler* otaWebHandler = nullptr;
};
