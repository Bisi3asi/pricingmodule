#include "logger.hpp"
#include <spdlog/sinks/null_sink.h>

namespace logger {

    /* 디폴트 로거 생성 차단을 위한 로거 초기화 */
    void disableConsoleLogging() {
        auto null_logger = std::make_shared<spdlog::logger>(
            "null_logger", std::make_shared<spdlog::sinks::null_sink_mt>());
        spdlog::set_default_logger(null_logger);
    }

    /* 로거 생성 */
    void initLogger(const std::string& filename) {
        try {
            std::filesystem::path log_dir = "../logs"; // 로깅 디렉토리 지정
            if (!std::filesystem::exists(log_dir)) {
                std::filesystem::create_directories(log_dir); // 해당 디렉토리 없을 시 생성
            }

            std::string full_path = std::filesystem::weakly_canonical(log_dir / filename).string();
            std::cout << "Log file will be created at: " << full_path << std::endl;

            if (!spdlog::get("logger")) {
                auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(full_path, /*truncate=*/true);
                auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

                std::vector<spdlog::sink_ptr> sinks{ file_sink, console_sink };
                auto combined_logger = std::make_shared<spdlog::logger>("logger", begin(sinks), end(sinks));

                spdlog::set_default_logger(combined_logger);
                spdlog::set_level(spdlog::level::info);
                spdlog::set_pattern("[%l][%Y-%m-%d %H:%M:%S] %v");
            }
        }
        catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Failed to create logger: " << ex.what() << std::endl;
        }
    }

    /* 로거 종료 및 해제 */
    void closeLogger() {
        try {
            spdlog::shutdown();  // 모든 로거 정리 및 해제
        }
        catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Failed to close logger: " << ex.what() << std::endl;
        }
    }

} // namespace logger
