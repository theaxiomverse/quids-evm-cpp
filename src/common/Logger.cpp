#include "common/Logger.hpp"
#include <stdexcept>

namespace quids::common {
    std::shared_ptr<spdlog::logger> Logger::logger_;

    void Logger::init(const std::string& name, 
                     const std::string& logfile,
                     bool console,
                     bool async) {
        std::vector<spdlog::sink_ptr> sinks;
        
        if(console) {
            sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        }
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logfile));

        if(async) {
            spdlog::init_thread_pool(8192, 1);
            logger_ = std::make_shared<spdlog::async_logger>(
                name, sinks.begin(), sinks.end(),
                spdlog::thread_pool(),
                spdlog::async_overflow_policy::block
            );
        } else {
            logger_ = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
        }

        logger_->set_level(spdlog::level::info);
        logger_->flush_on(spdlog::level::warn);
        spdlog::register_logger(logger_);
    }

    void Logger::set_level(spdlog::level::level_enum level) {
        instance()->set_level(level);
    }

    std::shared_ptr<spdlog::logger> Logger::instance() {
        if(!logger_) {
            throw std::runtime_error("Logger not initialized. Call Logger::init() first");
        }
        return logger_;
    }
} 