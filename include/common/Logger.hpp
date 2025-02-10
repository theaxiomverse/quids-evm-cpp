#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>

namespace quids::common {
    class Logger {
    public:
        static void init(const std::string& name = "quids", 
                        const std::string& logfile = "quids.log",
                        bool console = true,
                        bool async = true);
        
        template<typename... Args>
        static void info(const char* fmt, const Args&... args) {
            instance()->log(spdlog::level::info, fmt, args...);
        }
        
        // Similar methods for other levels
        // ... (warn, error, critical, debug, trace)

        static void set_level(spdlog::level::level_enum level);
        
    private:
        static std::shared_ptr<spdlog::logger> instance();
        static std::shared_ptr<spdlog::logger> logger_;
    };
} 