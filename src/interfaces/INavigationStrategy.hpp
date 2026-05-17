#pragma once
#include "domain/SensorData.hpp"
#include "domain/Direction.hpp"

class INavigationStrategy {
public:
    INavigationStrategy() = default;
    virtual ~INavigationStrategy() = default;
    INavigationStrategy(const INavigationStrategy&) = delete;
    INavigationStrategy& operator=(const INavigationStrategy&) = delete;
    INavigationStrategy(INavigationStrategy&&) = delete;
    INavigationStrategy& operator=(INavigationStrategy&&) = delete;

    virtual Direction navigate(const SensorData& data) = 0;
};
