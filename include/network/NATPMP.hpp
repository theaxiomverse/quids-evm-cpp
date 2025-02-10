#pragma once
#include <cstdint>

namespace quids::network {
    class NATPMP {
    public:
        enum class Protocol { UDP, TCP };
        
        static bool map_port(uint16_t internal_port, uint16_t external_port,
                           Protocol proto = Protocol::UDP, uint32_t lifetime = 3600);
    };
} 