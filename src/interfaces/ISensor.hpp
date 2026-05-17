#pragma once

class ISensor {
public:
    ISensor() = default;
    virtual ~ISensor() = default;
    ISensor(const ISensor&) = delete;
    ISensor& operator=(const ISensor&) = delete;
    ISensor(ISensor&&) = delete;
    ISensor& operator=(ISensor&&) = delete;

    virtual bool detect() = 0;
};
