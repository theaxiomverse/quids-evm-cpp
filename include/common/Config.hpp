#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace quids::common {
    class Config {
    public:
        explicit Config(const std::string& path = "config.json");
        
        template<typename T>
        T get(const std::string& key) const {
            return data_.value<T>(key, T{});
        }
        
        template<typename T>
        T get_or_default(const std::string& key, T default_val) const {
            return data_.value(key, default_val);
        }
        
        void reload();
        void validate() const;

    private:
        void load();
        
        std::string config_path_;
        nlohmann::json data_;
    };
} 