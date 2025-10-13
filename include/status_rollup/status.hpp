#pragma once

#include <cstdint>
#include <string_view>

namespace status_rollup {

// Status enumeration with ordering (Green < Yellow < Red)
enum class Status : uint8_t {
    Green = 0,
    Yellow = 1,
    Red = 2,
    Unknown = 3
};

// Convert string to Status
constexpr Status string_to_status(std::string_view s) {
    if (s == "green") return Status::Green;
    if (s == "yellow") return Status::Yellow;
    if (s == "red") return Status::Red;
    return Status::Unknown;
}

// Convert Status to string
constexpr std::string_view status_to_string(Status s) {
    switch (s) {
        case Status::Green: return "green";
        case Status::Yellow: return "yellow";
        case Status::Red: return "red";
        case Status::Unknown: return "unknown";
    }
    return "unknown";
}

} // namespace status_rollup
