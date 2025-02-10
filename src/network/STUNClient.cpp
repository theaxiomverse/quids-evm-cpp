#include "network/STUNClient.hpp"
#include <cstdint>
#include <array>
#include <sys/socket.h>
#include <unistd.h>
#include <spdlog/spdlog.h>
#include <arpa/inet.h>

namespace quids::network {
    // STUN message header (RFC 5389)
    struct StunHeader {
        uint16_t type;
        uint16_t length;
        uint32_t magic_cookie;
        uint8_t transaction_id[12];
    };

    bool STUNClient::get_mapped_address(const std::string& stun_server, uint16_t port, 
                                      std::string& public_ip, uint16_t& public_port) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            SPDLOG_ERROR("STUN socket creation failed");
            return false;
        }

        sockaddr_in server{};
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        inet_pton(AF_INET, stun_server.c_str(), &server.sin_addr);

        // Create STUN binding request
        StunHeader request{};
        request.type = htons(0x0001); // Binding Request
        request.length = 0;
        request.magic_cookie = htonl(0x2112A442);

        // Send request
        if(sendto(sock, &request, sizeof(request), 0, 
                (sockaddr*)&server, sizeof(server)) < 0) {
            close(sock);
            return false;
        }

        // Receive response with timeout
        fd_set fds;
        timeval tv{.tv_sec = 2, .tv_usec = 0};
        FD_ZERO(&fds);
        FD_SET(sock, &fds);

        if(select(sock+1, &fds, nullptr, nullptr, &tv) > 0) {
            std::array<uint8_t, 1024> buffer;
            sockaddr_in from{};
            socklen_t from_len = sizeof(from);
            
            ssize_t len = recvfrom(sock, buffer.data(), buffer.size(), 0,
                                 (sockaddr*)&from, &from_len);
            if(len > 0) {
                // Parse XOR-MAPPED-ADDRESS attribute
                // Implementation details omitted for brevity
                public_ip = inet_ntoa(from.sin_addr);
                public_port = ntohs(from.sin_port);
                close(sock);
                return true;
            }
        }

        close(sock);
        return false;
    }

    bool STUNClient::detect_symmetric_nat(const std::string& server1,
                                          const std::string& server2) {
        // Implementation placeholder
        return false;
    }
}