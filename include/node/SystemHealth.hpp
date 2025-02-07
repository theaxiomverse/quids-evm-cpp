#pragma once
#include <cstdint>
#include <string>

namespace quids {

struct SystemHealth {
    double cpu_usage{0.0};
    double memory_usage{0.0};
    double disk_usage{0.0};
    double network_latency{0.0};
    uint64_t uptime{0};
    std::string status{"unknown"};
    bool is_syncing{false};
    bool is_healthy{true};
};

} // namespace quids 