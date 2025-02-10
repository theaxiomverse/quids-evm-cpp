#include "network/NATPMP.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

namespace quids::network {
    bool NATPMP::map_port(uint16_t internal_port, uint16_t external_port, 
                        Protocol proto, uint32_t lifetime) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) return false;

        sockaddr_in gateway{};
        gateway.sin_family = AF_INET;
        gateway.sin_port = htons(5351); // NAT-PMP port
        inet_pton(AF_INET, "192.168.0.1", &gateway.sin_addr); // Typical gateway

        // Create NAT-PMP request
        struct {
            uint8_t version;
            uint8_t opcode;
            uint16_t reserved;
            uint16_t internal_port;
            uint16_t external_port;
            uint32_t lifetime;
        } request{};

        request.version = 0;
        request.opcode = proto == Protocol::TCP ? 2 : 1;
        request.internal_port = htons(internal_port);
        request.external_port = htons(external_port);
        request.lifetime = htonl(lifetime);

        // Send request and receive response
        // ... (full implementation would handle response parsing)

        close(sock);
        return true;
    }
}
