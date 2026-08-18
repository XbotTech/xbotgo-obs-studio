#pragma once
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <functional>

namespace blink {
namespace media {

    enum BL_LOG_LEVEL {
        BL_LOG_DEBUG = 0,
        BL_LOG_INFO,
        BL_LOG_WARN,
        BL_LOG_ERROR,
        BL_LOG_LEVEL_NUM
    };

    typedef std::function<void(int level, const std::string &tag, const std::string &msg)> write_log_func;
    // void init_logger(const char* log_dir, const char* log_name,
    //   int32_t max_log_count = 5, int32_t max_log_size = (5 << 20));
    void init_logger(write_log_func cb);
    void log(BL_LOG_LEVEL level, const char *tag, const char* fmt, ...);
    //std::string get_log_path();
}
}
