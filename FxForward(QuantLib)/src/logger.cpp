#include "logger.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <filesystem>
#include <iostream>

/* 파일 로거를 생성한다. */
void initLogger(const std::string& filename) {
    try {
        std::string full_path = std::filesystem::weakly_canonical("../logs/" + filename).string();
        std::cout << "Log file will be created at: " << full_path << std::endl;

        if (!spdlog::get("logger")) {
            auto file_logger = spdlog::basic_logger_mt("logger", full_path, true);
            spdlog::set_default_logger(file_logger);
            spdlog::set_level(spdlog::level::info);
            spdlog::set_pattern("[%l][%Y-%m-%d %H:%M:%S] %v");
        }
    }
    catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Failed to create logger: " << ex.what() << std::endl;
    }

}
