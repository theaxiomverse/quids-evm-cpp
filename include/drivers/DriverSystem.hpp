#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include "quantum/QuantumCrypto.hpp"

namespace drivers {

// Forward declarations
class Driver;
class Protocol;
class Device;

class DriverSystem {
public:
    // Driver types
    enum class DriverType {
        BLOCKCHAIN,
        HARDWARE,
        PROTOCOL,
        NETWORK
    };

    struct DriverConfig {
        std::string name;
        DriverType type;
        std::string version;
        std::unordered_map<std::string, std::string> parameters;
    };

    struct ConnectionInfo {
        std::string endpoint;
        std::string protocol;
        std::unordered_map<std::string, std::string> auth_params;
        bool encrypted;
    };

    // Constructor
    DriverSystem();
    ~DriverSystem();

    // Driver management
    bool registerDriver(std::shared_ptr<Driver> driver);
    bool unregisterDriver(const std::string& driver_name);
    std::shared_ptr<Driver> getDriver(const std::string& driver_name);

    // Device management
    bool connectDevice(const std::string& device_id, const ConnectionInfo& connection);
    bool disconnectDevice(const std::string& device_id);
    std::shared_ptr<Device> getDevice(const std::string& device_id);

    // Protocol handling
    bool registerProtocol(std::shared_ptr<Protocol> protocol);
    bool unregisterProtocol(const std::string& protocol_name);
    std::shared_ptr<Protocol> getProtocol(const std::string& protocol_name);

    // Event handling
    using EventCallback = std::function<void(const std::string& device_id,
                                           const std::string& event_type,
                                           const std::vector<uint8_t>& data)>;
    
    void registerEventHandler(const std::string& event_type, EventCallback handler);
    void unregisterEventHandler(const std::string& event_type);

    // Status and monitoring
    struct DriverStatus {
        bool connected;
        std::string status;
        uint64_t last_active;
        std::vector<std::string> active_connections;
    };

    DriverStatus getDriverStatus(const std::string& driver_name);
    std::vector<std::string> listActiveDrivers();
    std::vector<std::string> listConnectedDevices();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    // Internal helper methods
    bool validateDriver(const std::shared_ptr<Driver>& driver);
    bool validateConnection(const ConnectionInfo& connection);
    void handleDeviceEvent(const std::string& device_id,
                          const std::string& event_type,
                          const std::vector<uint8_t>& data);
};

// Base class for all drivers
class Driver {
public:
    virtual ~Driver() = default;
    virtual bool initialize(const DriverSystem::DriverConfig& config) = 0;
    virtual bool connect() = 0;
    virtual bool disconnect() = 0;
    virtual std::vector<uint8_t> sendCommand(const std::vector<uint8_t>& command) = 0;
    virtual DriverSystem::DriverStatus getStatus() = 0;
};

// Base class for all protocols
class Protocol {
public:
    virtual ~Protocol() = default;
    virtual bool initialize() = 0;
    virtual std::vector<uint8_t> encode(const std::vector<uint8_t>& data) = 0;
    virtual std::vector<uint8_t> decode(const std::vector<uint8_t>& data) = 0;
};

// Base class for all devices
class Device {
public:
    virtual ~Device() = default;
    virtual bool connect(const DriverSystem::ConnectionInfo& info) = 0;
    virtual bool disconnect() = 0;
    virtual bool isConnected() = 0;
    virtual std::vector<uint8_t> sendData(const std::vector<uint8_t>& data) = 0;
};

} // namespace drivers 