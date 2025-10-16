#include "logger.hpp"

#include <spdlog/sinks/null_sink.h>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <vector>

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
            namespace fs = std::filesystem;

            // 로깅 디렉토리 지정 및 생성
            fs::path log_dir = "./logs";
            if (!fs::exists(log_dir)) {
                fs::create_directories(log_dir); // 해당 디렉토리 없을 시 생성
            }

            // 로깅 파일에 타임스탬프 추가 (밀리초 포함)
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
            // 타임스탬프 형식: YYYYMMDD.HHMMSS.mmm
            tsoss << std::put_time(&local_tm, "%Y%m%d") << '.'
                  << std::put_time(&local_tm, "%H%M%S") << '.'
                  << std::setw(3) << std::setfill('0') << ms.count();

            fs::path in_path = filename;
            std::string base = in_path.filename().string();
            std::string base_with_ts = base + "_" + tsoss.str();

            // 기본 파일명은 ...-1.log 로 시작
            // 만약 동일한 파일이 존재하면 -2, -3 ... 식으로 증가시켜 사용
            // 밀리초가 바뀌면 base_with_ts가 달라지므로 다시 -1로 시작
            int idx = 1;
            fs::path chosen_path;
            while (true) {
                std::string candidate = base_with_ts + "-" + std::to_string(idx) + ".log";
                fs::path candidate_path = log_dir / candidate;
                if (!fs::exists(candidate_path)) {
                    chosen_path = candidate_path;
                    break;
                }
                ++idx;
            }

            std::string full_path = fs::weakly_canonical(chosen_path).string();
            std::cout << "Log file will be created at: " << full_path << std::endl;
            std::cout << "If Log file is not created, please check the permissions of the 'logs' directory." << std::endl;
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
                error("Pricing Process Fail, Exit with Error Code: {}.", result);
            }
            info("Logging End.");
            spdlog::shutdown();  // 모든 로거 정리 및 해제
        }
        catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Failed to close logger: " << ex.what() << std::endl;
        }
    }

} // namespace logger
