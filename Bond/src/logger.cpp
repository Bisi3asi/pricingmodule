#include "logger.h"
#include <spdlog/sinks/null_sink.h>

/* 디폴트 로거 차단을 위한 null sink 로거 생성 */
void disableConsoleLogging() {
    auto null_logger = std::make_shared<spdlog::logger>("null_logger", std::make_shared<spdlog::sinks::null_sink_mt>());
    spdlog::set_default_logger(null_logger);
}

/* 파일 로거를 생성한다. */
void initLogger(const std::string& filename) {
    try {
        std::filesystem::path log_dir = "../logs";
        if (!std::filesystem::exists(log_dir)) {
            std::filesystem::create_directories(log_dir); // 디렉토리 생성
        }

        std::string full_path = std::filesystem::weakly_canonical(log_dir/filename).string();
        std::cout << "Log file will be created at: " << full_path << std::endl;

        if (!spdlog::get("logger")) {
            auto file_logger = spdlog::basic_logger_mt("logger", full_path, false);
            spdlog::set_default_logger(file_logger);
            spdlog::set_level(spdlog::level::info);
            spdlog::set_pattern("[%l][%Y-%m-%d %H:%M:%S] %v");
        }
    }
    catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Failed to create logger: " << ex.what() << std::endl;
    }
}