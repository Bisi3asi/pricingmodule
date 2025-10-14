#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <string>
#include <vector>
#include <iomanip>
#include <iostream>
#include <filesystem>

namespace logger {

    void disableConsoleLogging();
    void initLogger(const std::string& filename, const char* funcName = nullptr);
    void closeLogger(const char* funcName = nullptr);

    // 기존 포맷팅 지원 템플릿
    template <typename... Args>
    void info(fmt::format_string<Args...> fmt, Args&&... args) {
        if (auto logger = spdlog::default_logger()) {
            logger->info(fmt, std::forward<Args>(args)...);
        }
    }

    template <typename... Args>
    void error(fmt::format_string<Args...> fmt, Args&&... args) {
        if (auto logger = spdlog::default_logger()) {
            logger->error(fmt, std::forward<Args>(args)...);
        }
    }

    /**/
    // 오버로드: 단순 문자열(const char*) 로그용
    inline void info(const char* msg) {
        if (auto logger = spdlog::default_logger()) {
            logger->info(msg);
        }
    }

    inline void error(const char* msg) {
        if (auto logger = spdlog::default_logger()) {
            logger->error(msg);
        }
    }

    // 오버로드: 단순 문자열(std::string) 로그용
    inline void info(const std::string& msg) {
        if (auto logger = spdlog::default_logger()) {
            logger->info(msg);
        }
    }

    inline void error(const std::string& msg) {
        if (auto logger = spdlog::default_logger()) {
            logger->error(msg);
        }
    }

    // 배열/벡터 출력 헬퍼
    template <typename T>
    void logArrayLine(const std::string& label, const T* data, size_t size, int precision = -1) {
        std::ostringstream oss;
        if (precision >= 0)
            oss << std::fixed << std::setprecision(precision);
        oss << label << ": ";
        for (size_t i = 0; i < size; ++i) {
            oss << data[i];
            if (i != size - 1)
                oss << ", ";
        }
        info(oss.str());
    }

    template <typename T>
    void logArrayLine(const std::string& label, const std::vector<T>& data, int precision = -1) {
        logArrayLine(label, data.data(), data.size(), precision);
    }
}

// 로깅용 매크로 정의
#define LOG_START(fileName) logger::initLogger(fileName, __func__)
#define LOG_END() logger::closeLogger(__func__)
#define LOG_VAR(x) logger::info("{} = {}", #x, x) // 변수명 = 값
