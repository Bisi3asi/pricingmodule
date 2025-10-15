#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <string>
#include <vector>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <type_traits> 

#define LOG_START(fileName) logger::initLogger(fileName, __func__)
#define LOG_END(result) logger::closeLogger(result)
#define LOG_VAR(x) logger::Loggable<decltype(x)>::log(#x, (x))
#define LOG_ARRAY(ptr, size, ...) logger::logArrayLine(#ptr "[]", (ptr), (size), ##__VA_ARGS__)
#define LOG_ENTER_PRICING() logger::enterPricing()
#define LOG_KNOWN_EXCEPTION_ERROR(msg) logger::logKnownExceptionError(msg)
#define LOG_UNKNOWN_EXCEPTION_ERROR() logger::logUnknownExceptionError()

// 배열 로깅을 위한 헬퍼 구조체
struct ArrayInt {
    const int* data;
    size_t size;
};

struct ArrayDouble {
    const double* data;
    size_t size;
    int precision = -1;
};

namespace logger {

    void disableConsoleLogging();
    void initLogger(const std::string& filename, const char* funcName = nullptr);
    void closeLogger(const double result);

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

    inline void enterPricing() {
        info("Pricing Process Start.");
        info("");
    }

    inline void logKnownExceptionError(const std::string& msg) {
        error("! Exception Occured !");
        error("---------------------------------------------");
        error("{}", msg);
        error("---------------------------------------------");
        info("");
    }

    inline void logUnknownExceptionError() {
        error("! Exception Occured !");
        error("---------------------------------------------");
        error("Unknown Exception, please check the input parameters and environment.");
        error("---------------------------------------------");
        info("");
    }

    // 배열/벡터 출력 헬퍼
    template <typename T>
    void logArrayLine(const std::string& label, const T* data, size_t size, int precision = 10) {
        std::ostringstream oss;
        if (precision >= 0)
            oss << std::fixed << std::setprecision(precision);
        oss << label << " = {";
        for (size_t i = 0; i < size; ++i) {
            oss << data[i];
            if (i != size - 1)
                oss << ", ";
        }
        oss << "};";
        info("{}", oss.str());
    }

    template <typename T>
    void logArrayLine(const std::string& label, const std::vector<T>& data, int precision = 10) {
        logArrayLine(label, data.data(), data.size(), precision);
    }

    // 타입 이름을 문자열로 변환하는 헬퍼 
    template <typename T>
    constexpr const char* type_name() {
        using U = std::remove_cv_t<std::remove_reference_t<T>>;
        if constexpr (std::is_same_v<U, int>) return "int";
        if constexpr (std::is_same_v<U, double>) return "double";
        if constexpr (std::is_same_v<U, const char*>) return "const char*";
        if constexpr (std::is_same_v<U, std::string>) return "std::string";
        if constexpr (std::is_same_v<U, ArrayInt>) return "int[]";
        if constexpr (std::is_same_v<U, ArrayDouble>) return "double[]";
        return "unknown_type";
    }

    template<typename T>
    struct Loggable {
        static void log(const char* name, const T& value) {
            // 예: double evaluationDate = 45107;
            info("{} {} = {};", type_name<T>(), name, value);
        }
    };

} // namespace logger


