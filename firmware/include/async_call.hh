#pragma once
#include <functional>

void asyncCall(std::function<void()> callback, const uint32_t usStackDepth, const uint32_t delayMs);
