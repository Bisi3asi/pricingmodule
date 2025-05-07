#include "logger.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <filesystem>

/* 파일 로거를 생성한다. */
void initLogger(const std::string& filename) {
    std::filesystem::create_directories("../logs");

    if (!spdlog::get("logger")) {
        auto file_logger = spdlog::basic_logger_mt("logger", "../logs/" + filename, true); // overwrite : true
        spdlog::set_default_logger(file_logger);
        spdlog::set_level(spdlog::level::info);
        spdlog::set_pattern("[%l][%Y-%m-%d %H:%M:%S] %v");
    }
}
