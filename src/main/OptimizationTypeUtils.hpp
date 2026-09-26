#pragma once
#include <string>
#include "OptimizationProfileDatabase.hpp"

// Utility functions for converting optimization enums to strings
class OptimizationTypeUtils {
public:
    // Convert FileSize enum to string
    static std::string fileSizeToString(OptimizationContext::FileSize fs) {
        switch (fs) {
            case OptimizationContext::FileSize::TINY: return "TINY";
            case OptimizationContext::FileSize::SMALL: return "SMALL";
            case OptimizationContext::FileSize::MEDIUM: return "MEDIUM";
            case OptimizationContext::FileSize::LARGE: return "LARGE";
            case OptimizationContext::FileSize::HUGE: return "HUGE";
        }
        return "UNKNOWN";
    }

    // Convert Complexity enum to string
    static std::string complexityToString(OptimizationContext::Complexity c) {
        switch (c) {
            case OptimizationContext::Complexity::LOW: return "LOW";
            case OptimizationContext::Complexity::MODERATE: return "MODERATE";
            case OptimizationContext::Complexity::HIGH: return "HIGH";
            case OptimizationContext::Complexity::VERY_HIGH: return "VERY_HIGH";
        }
        return "UNKNOWN";
    }

private:
    OptimizationTypeUtils() = delete;  // Static utility class
};
