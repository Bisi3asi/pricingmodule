#include "logger.hpp"

#include <spdlog/sinks/null_sink.h>
#include <chrono>
#include <ctime>

namespace logger {

    /* 디폴트 로거 생성 차단을 위한 로거 초기화 */
    void disableConsoleLogging() {
        auto null_logger = std::make_shared<spdlog::logger>(
            "null_logger", std::make_shared<spdlog::sinks::null_sink_mt>());
        spdlog::set_default_logger(null_logger);
    }

    /* 로거 생성 */
    void initLogger(const std::string& filename, const char* funcName) {
        try {
            std::filesystem::path log_dir = "./logs"; // 로깅 디렉토리 지정
            if (!std::filesystem::exists(log_dir)) {
                std::filesystem::create_directories(log_dir); // 해당 디렉토리 없을 시 생성
            }
        
            // 로깅 파일에 타임스탬프 추가
            auto now = std::chrono::system_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
            std::time_t tnow = std::chrono::system_clock::to_time_t(now);
            std::tm local_tm;

            #if defined(_MSC_VER) // MSVC
                localtime_s(&local_tm, &tnow);
            #else // Linux/Unix
                localtime_r(&tnow, &local_tm);
            #endif

            std::ostringstream tsoss;
            // 타임스탬프 형식 ex) 20251015.123045.123
            tsoss << std::put_time(&local_tm, "%Y%m%d") << '.'
                << std::put_time(&local_tm, "%H%M%S") << '.'
                << std::setw(3) << std::setfill('0') << ms.count();

            std::filesystem::path in_path = filename;
            std::string base = in_path.filename().string();
            std::string out_name = base + "_" + tsoss.str() + ".log";

            std::string full_path = std::filesystem::weakly_canonical(log_dir / out_name).string();
            std::cout << "Log file will be created at: " << full_path << std::endl;
            std::cout << std::endl;

            if (!spdlog::get("logger")) {
                auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(full_path, /*truncate=*/true);
                auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

                std::vector<spdlog::sink_ptr> sinks{ file_sink, console_sink };
                auto combined_logger = std::make_shared<spdlog::logger>("logger", begin(sinks), end(sinks));

                spdlog::set_default_logger(combined_logger);
                spdlog::set_level(spdlog::level::info);
                spdlog::set_pattern("[%l][%Y-%m-%d %H:%M:%S] %v");
            }

            if (funcName) {
                info("[{}] - Logging Start.", funcName);
                info("");
            }
        }
        catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Failed to create logger: " << ex.what() << std::endl;
        }
    }

    /* 로거 종료 및 해제 */
    void closeLogger(const double result) {
        try {
            if (result >= 0.0) {
                info("Pricing Process Success.");
            }
            else {
                error("Pricing Process Fail with Errors: {}.", result);
            }
            info("Logging End.");
            spdlog::shutdown();  // 모든 로거 정리 및 해제
        }
        catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Failed to close logger: " << ex.what() << std::endl;
        }
    }

} // namespace logger
