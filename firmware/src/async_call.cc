#include "async_call.hh"

#include <esp32-hal.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h> // NOLINT
#include <freertos/task.h>

struct AsyncCallArgs
{
    std::function<void()> callback;
    uint32_t delayMs;
};

void doCall(void* args) // NOLINT
{
    const auto restartArgs = static_cast<const AsyncCallArgs*>(args);
    delay(restartArgs->delayMs);
    restartArgs->callback();
    delete restartArgs;
    vTaskDelete(nullptr);
}

void asyncCall(std::function<void()> callback, const uint32_t usStackDepth, const uint32_t delayMs)
{
    const auto args = new AsyncCallArgs{std::move(callback), delayMs};
    if (xTaskCreate(doCall, "AsyncCallTask", usStackDepth, args, 1, nullptr) != pdPASS)
    {
        ESP_LOGE("asyncCall", "Failed to create task");
        delete args;
    }
}
