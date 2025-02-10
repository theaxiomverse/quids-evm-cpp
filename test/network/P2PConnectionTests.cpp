#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "network/P2PConnection.hpp"
#include <thread>
#include <future>

using namespace quids::network;
using namespace testing;

class P2PConnectionTest : public Test {
protected:
    void SetUp() override {
        config1_.port = 12345;
        config1_.stun_server = "stun.l.google.com";
        config1_.stun_port = 19302;
        config1_.enable_upnp = true;
        config1_.enable_nat_pmp = true;
        config1_.max_peers = 10;
        config1_.hole_punch_timeout = std::chrono::milliseconds(1000);
        config1_.keep_alive_interval = std::chrono::milliseconds(5000);

        config2_.port = 12346;
        config2_.stun_server = "stun.l.google.com";
        config2_.stun_port = 19302;
        config2_.enable_upnp = true;
        config2_.enable_nat_pmp = true;
        config2_.max_peers = 10;
        config2_.hole_punch_timeout = std::chrono::milliseconds(1000);
        config2_.keep_alive_interval = std::chrono::milliseconds(5000);
    }

    P2PConnection::Config config1_;
    P2PConnection::Config config2_;
};

TEST_F(P2PConnectionTest, StartAndStop) {
    P2PConnection connection(config1_);
    EXPECT_TRUE(connection.start());
    connection.stop();
}

TEST_F(P2PConnectionTest, GetPublicEndpoint) {
    P2PConnection connection(config1_);
    ASSERT_TRUE(connection.start());

    auto [ip, port] = connection.get_public_endpoint();
    EXPECT_FALSE(ip.empty());
    EXPECT_GT(port, 0);

    connection.stop();
}

TEST_F(P2PConnectionTest, PeerConnection) {
    P2PConnection connection1(config1_);
    P2PConnection connection2(config2_);

    ASSERT_TRUE(connection1.start());
    ASSERT_TRUE(connection2.start());

    std::promise<bool> connected_promise;
    auto connected_future = connected_promise.get_future();

    connection2.set_peer_connected_handler([&](const std::string& peer_id) {
        connected_promise.set_value(true);
    });

    auto [ip1, port1] = connection1.get_public_endpoint();
    ASSERT_FALSE(ip1.empty());
    ASSERT_GT(port1, 0);

    EXPECT_TRUE(connection2.perform_nat_traversal(ip1, port1));

    // Wait for connection with timeout
    auto status = connected_future.wait_for(std::chrono::seconds(5));
    EXPECT_EQ(status, std::future_status::ready);
    EXPECT_TRUE(connected_future.get());

    connection1.stop();
    connection2.stop();
}

TEST_F(P2PConnectionTest, MessageExchange) {
    P2PConnection connection1(config1_);
    P2PConnection connection2(config2_);

    ASSERT_TRUE(connection1.start());
    ASSERT_TRUE(connection2.start());

    std::promise<std::vector<uint8_t>> message_promise;
    auto message_future = message_promise.get_future();

    connection2.set_message_handler([&](const std::string& peer_id, const std::vector<uint8_t>& data) {
        message_promise.set_value(data);
    });

    auto [ip1, port1] = connection1.get_public_endpoint();
    ASSERT_FALSE(ip1.empty());
    ASSERT_GT(port1, 0);

    EXPECT_TRUE(connection2.perform_nat_traversal(ip1, port1));

    // Wait for connection establishment
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Send test message
    std::vector<uint8_t> test_message = {'H', 'e', 'l', 'l', 'o'};
    connection1.broadcast(test_message);

    // Wait for message with timeout
    auto status = message_future.wait_for(std::chrono::seconds(5));
    EXPECT_EQ(status, std::future_status::ready);
    EXPECT_EQ(message_future.get(), test_message);

    connection1.stop();
    connection2.stop();
}

TEST_F(P2PConnectionTest, PeerDisconnection) {
    P2PConnection connection1(config1_);
    P2PConnection connection2(config2_);

    ASSERT_TRUE(connection1.start());
    ASSERT_TRUE(connection2.start());

    std::promise<std::string> disconnected_promise;
    auto disconnected_future = disconnected_promise.get_future();

    connection2.set_peer_disconnected_handler([&](const std::string& peer_id) {
        disconnected_promise.set_value(peer_id);
    });

    auto [ip1, port1] = connection1.get_public_endpoint();
    ASSERT_FALSE(ip1.empty());
    ASSERT_GT(port1, 0);

    EXPECT_TRUE(connection2.perform_nat_traversal(ip1, port1));

    // Wait for connection establishment
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Stop first connection
    connection1.stop();

    // Wait for disconnection notification with timeout
    auto status = disconnected_future.wait_for(std::chrono::seconds(10));
    EXPECT_EQ(status, std::future_status::ready);
    EXPECT_FALSE(disconnected_future.get().empty());

    connection2.stop();
}

TEST_F(P2PConnectionTest, BroadcastToMultiplePeers) {
    P2PConnection connection1(config1_);
    P2PConnection connection2(config2_);
    
    P2PConnection::Config config3 = config1_;
    config3.port = 12347;
    P2PConnection connection3(config3);

    ASSERT_TRUE(connection1.start());
    ASSERT_TRUE(connection2.start());
    ASSERT_TRUE(connection3.start());

    std::promise<std::vector<uint8_t>> message_promise2;
    std::promise<std::vector<uint8_t>> message_promise3;
    auto message_future2 = message_promise2.get_future();
    auto message_future3 = message_promise3.get_future();

    connection2.set_message_handler([&](const std::string& peer_id, const std::vector<uint8_t>& data) {
        message_promise2.set_value(data);
    });

    connection3.set_message_handler([&](const std::string& peer_id, const std::vector<uint8_t>& data) {
        message_promise3.set_value(data);
    });

    auto [ip1, port1] = connection1.get_public_endpoint();
    ASSERT_FALSE(ip1.empty());
    ASSERT_GT(port1, 0);

    EXPECT_TRUE(connection2.perform_nat_traversal(ip1, port1));
    EXPECT_TRUE(connection3.perform_nat_traversal(ip1, port1));

    // Wait for connections to establish
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Send test message
    std::vector<uint8_t> test_message = {'H', 'e', 'l', 'l', 'o'};
    connection1.broadcast(test_message);

    // Wait for messages with timeout
    auto status2 = message_future2.wait_for(std::chrono::seconds(5));
    auto status3 = message_future3.wait_for(std::chrono::seconds(5));
    
    EXPECT_EQ(status2, std::future_status::ready);
    EXPECT_EQ(status3, std::future_status::ready);
    EXPECT_EQ(message_future2.get(), test_message);
    EXPECT_EQ(message_future3.get(), test_message);

    connection1.stop();
    connection2.stop();
    connection3.stop();
}

TEST_F(P2PConnectionTest, DirectMessageToPeer) {
    P2PConnection connection1(config1_);
    P2PConnection connection2(config2_);

    ASSERT_TRUE(connection1.start());
    ASSERT_TRUE(connection2.start());

    std::promise<std::pair<std::string, std::vector<uint8_t>>> message_promise;
    auto message_future = message_promise.get_future();

    connection2.set_message_handler([&](const std::string& peer_id, const std::vector<uint8_t>& data) {
        message_promise.set_value({peer_id, data});
    });

    auto [ip1, port1] = connection1.get_public_endpoint();
    ASSERT_FALSE(ip1.empty());
    ASSERT_GT(port1, 0);

    EXPECT_TRUE(connection2.perform_nat_traversal(ip1, port1));

    // Wait for connection establishment
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Get peer ID
    auto peers = connection1.get_connected_peers();
    ASSERT_FALSE(peers.empty());
    std::string peer_id = peers[0].id;

    // Send direct message
    std::vector<uint8_t> test_message = {'D', 'i', 'r', 'e', 'c', 't'};
    connection1.send_to_peer(peer_id, test_message);

    // Wait for message with timeout
    auto status = message_future.wait_for(std::chrono::seconds(5));
    EXPECT_EQ(status, std::future_status::ready);
    
    auto [received_peer_id, received_message] = message_future.get();
    EXPECT_FALSE(received_peer_id.empty());
    EXPECT_EQ(received_message, test_message);

    connection1.stop();
    connection2.stop();
}

TEST_F(P2PConnectionTest, PeerManagement) {
    P2PConnection connection1(config1_);
    P2PConnection connection2(config2_);

    ASSERT_TRUE(connection1.start());
    ASSERT_TRUE(connection2.start());

    auto [ip1, port1] = connection1.get_public_endpoint();
    ASSERT_FALSE(ip1.empty());
    ASSERT_GT(port1, 0);

    EXPECT_TRUE(connection2.perform_nat_traversal(ip1, port1));

    // Wait for connection establishment
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Check connected peers
    auto peers = connection1.get_connected_peers();
    EXPECT_FALSE(peers.empty());

    // Check specific peer connection
    EXPECT_TRUE(connection1.is_peer_connected(peers[0].id));

    // Disconnect peer
    connection1.disconnect_peer(peers[0].id);
    EXPECT_FALSE(connection1.is_peer_connected(peers[0].id));

    connection1.stop();
    connection2.stop();
} 