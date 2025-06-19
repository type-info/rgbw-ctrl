
## ⏳ Async Call

`async_call` is a utility function to execute a callback after a delay using a separate FreeRTOS task.

### Purpose

Allows scheduling any `std::function<void()>` to run after a specified time without blocking the current task.

### Behavior

* Creates a task named "AsyncCallTask"
* Waits for the configured delay using `vTaskDelayUntil()` for precise timing
* Executes the given callback
* Deletes the task automatically

### Parameters

```cpp
void async_call(
    std::function<void()> callback,
    uint32_t usStackDepth,
    uint32_t delayMs)
```

* `callback`: function to execute after the delay
* `usStackDepth`: stack size for the new task
* `delayMs`: delay duration in milliseconds

### Usage Example

```cpp
async_call([] {
    Serial.println("Executed after 2 seconds");
}, 2048, 2000);
```

Ideal for debounce mechanisms, visual timers, or non-blocking delays in UI or sensor workflows.
