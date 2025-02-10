#include "common/Config.hpp"
#include <fstream>
#include <stdexcept>

namespace quids::common {
    Config::Config(const std::string& path) : config_path_(path) {
        load();
    }

    void Config::load() {
        std::ifstream f(config_path_);
        if(!f.is_open()) {
            throw std::runtime_error("Config file not found: " + config_path_);
        }
        try {
            data_ = nlohmann::json::parse(f);
        } catch(const nlohmann::json::parse_error& e) {
            throw std::runtime_error("Config parse error: " + std::string(e.what()));
        }
    }

    void Config::reload() {
        load();
        validate();
    }

    void Config::validate() const {
        const std::vector<std::string> required = {
            "network.port",
            "network.stun_server",
            "storage.path"
        };
        
        for(const auto& key : required) {
            if(!data_.contains(nlohmann::json::json_pointer("/" + key))) {
                throw std::runtime_error("Missing required config key: " + key);
            }
        }
    }
} 