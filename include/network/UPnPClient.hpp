#pragma once
#include <string>

namespace quids::network {
    class UPnPClient {
    public:
        static bool add_port_mapping(uint16_t external_port, uint16_t internal_port,
                                   const std::string& protocol = "TCP",
                                   const std::string& description = "");
    };
} 