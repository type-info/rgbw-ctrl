#pragma once

template <typename T>
class ThrottledValue
{
    T lastValue;
    unsigned long lastSendTime = 0;
    const unsigned long throttleInterval;

public:
    explicit ThrottledValue(unsigned long intervalMs)
        : throttleInterval(intervalMs)
    {
    }

    bool shouldSend(const unsigned long now, const T& newValue)
    {
        if (newValue == lastValue || (now - lastSendTime) < throttleInterval)
            return false;
        return true;
    }

    void setLastSent(const unsigned long time, const T& value)
    {
        this->lastValue = value;
        this->lastSendTime = time;
    }

    const T& getLastValue() const { return lastValue; }
    unsigned long getLastSendTime() const { return lastSendTime; }
};
