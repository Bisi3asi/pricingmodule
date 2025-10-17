#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/cat.hpp>

#include <string>
#include <vector>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <type_traits>
#include <fmt/core.h>

#define LOG_START(fileName) logger::initLogger(fileName, __func__)
#define LOG_END(result) logger::closeLogger(result)
#define LOG_STRINGIFY_IMPL(x) #x
#define LOG_STRINGIFY(x) LOG_STRINGIFY_IMPL(x)
#define LOG_VAR(x) logger::Loggable<decltype(x)>::log(LOG_STRINGIFY(x), (x))
#define FIELD_VAR(x) (VAR, x)
#define FIELD_ARR(ptr, size) (ARR, ptr, size)
#define LOG_FIELDS_DISPATCH_VAR(tuple) LOG_VAR(BOOST_PP_TUPLE_ELEM(1, tuple))
#define LOG_FIELDS_DISPATCH_ARR(tuple) LOG_ARRAY(BOOST_PP_TUPLE_ELEM(1, tuple), BOOST_PP_TUPLE_ELEM(2, tuple))
#define LOG_FIELDS_DISPATCH_IMPL(tuple) BOOST_PP_CAT(LOG_FIELDS_DISPATCH_, BOOST_PP_TUPLE_ELEM(0, tuple))(tuple)
#define LOG_FIELDS_EACH(r, data, elem) LOG_FIELDS_DISPATCH_IMPL(elem);
#define LOG_FIELDS(...) BOOST_PP_SEQ_FOR_EACH(LOG_FIELDS_EACH, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
#define LOG_ARRAY(ptr, size, ...) logger::logArrayLine(LOG_STRINGIFY(ptr) "[]", (ptr), (size), ##__VA_ARGS__)

namespace logger {

    void disableConsoleLogging();
    void initLogger(const std::string& filename, const char* funcName = nullptr);
    void closeLogger(const double result);

    // 기존 포맷팅 지원 템플릿 함수
    template <typename... Args>
    void info(fmt::format_string<Args...> fmt, Args&&... args) {
        if (auto logger = spdlog::default_logger()) {
            logger->info(fmt, std::forward<Args>(args)...);
        }
    }

    // 호출자 파일/라인을 붙여서 로그를 남기는 헬퍼
    // Format: [<파일명>:<라인번호>] 메시지
    template <typename... Args>
    void infoWithLine(const char* file, int line, fmt::format_string<Args...> fmt, Args&&... args) {
        std::string body = fmt::format(fmt, std::forward<Args>(args)...);
        std::ostringstream oss;
        try {
            oss << "[" << std::filesystem::path(file).filename().string() << ":" << line << "] " << body;
        } catch (...) {
            oss.str(std::string());
            oss << "[" << file << ":" << line << "] " << body;
        }
        info("{}", oss.str());
    }

    // 기존 포맷팅 지원 템플릿 함수
    template <typename... Args>
    void error(fmt::format_string<Args...> fmt, Args&&... args) {
        if (auto logger = spdlog::default_logger()) {
            logger->error(fmt, std::forward<Args>(args)...);
        }
    }

    // errorWithLine: 호출자 파일/라인을 붙여서 에러 로그를 남기는 헬퍼
    // Format: [<파일명>:<라인번호>] 메시지
    template <typename... Args>
    void errorWithLine(const char* file, int line, fmt::format_string<Args...> fmt, Args&&... args) {
        std::string body = fmt::format(fmt, std::forward<Args>(args)...);
        std::ostringstream oss;
        try {
            oss << body << " (" << std::filesystem::path(file).filename().string() << ":" << line << ")";
        } catch (...) {
            oss.str(std::string());
            oss << body << " (" <<" << file << ":" << line << ")";
        }
        error("{}", oss.str());
    }

    // 오버로드: 단순 문자열(const char*) 로그용
    inline void info(const char* msg) {
        if (auto logger = spdlog::default_logger()) {
            logger->info(msg);
        }
    }

    // 오버로드: 단순 문자열(const char*) 로그용
    inline void error(const char* msg) {
        if (auto logger = spdlog::default_logger()) {
            logger->error(msg);
        }
    }

    // 오버로드: 단순 문자열(std::string) 로그용
    inline void info(const std::string& msg) { info(msg.c_str()); }
    inline void error(const std::string& msg) { error(msg.c_str()); }

    template <typename T>
    void logArrayLine(const std::string& label, const std::vector<T>& data, int precision = 10) {
        logArrayLine(label, data.data(), data.size(), precision);
    }

    // 타입 이름 문자열 변환 헬퍼 (used by Loggable)
    template <typename T>
    constexpr const char* type_name() {
        using U = std::remove_cv_t<std::remove_reference_t<T>>;
        if constexpr (std::is_same_v<U, int>) return "int";
        if constexpr (std::is_same_v<U, long>) return "long";
        if constexpr (std::is_same_v<U, long long>) return "long long";
        if constexpr (std::is_same_v<U, float>) return "float";
        if constexpr (std::is_same_v<U, double>) return "double";
        if constexpr (std::is_same_v<U, const char*>) return "const char*";
        // if constexpr (std::is_same_v<U, std::string>) return "std::string"; // string은 type_name에서 출력하지 않도록 임시 비활성화 처리
        return "unknown_type";
    }

    // 타입 포함 단순 변수 출력 헬퍼 (used by LOG_VAR)
    // Format: <타입 변수명> = <변수값>; 
    template<typename T>
    struct Loggable {
        static void log(const char* name, const T& value) {
            info("{} {} = {};", type_name<T>(), name, value);
        }
    };

    // 타입 포함 배열 / 벡터 출력 헬퍼 (used by LOG_ARRAY)
    // Format: <타입> <배열/벡터명>[] = {<변수값>};
    // Format: (all-zero): <타입> <배열/벡터명>[] = N/A;
    template <typename T>
    void logArrayLine(const std::string& label, const T* data, size_t size, int precision = 10) {
        std::ostringstream oss;
        if (precision >= 0)
            oss.setf(std::ios::fixed), oss.precision(precision);
        using U = std::remove_cv_t<std::remove_reference_t<T>>;

        // 숫자형 배열이 모두 0인 경우 미평가로, 간단히 "N/A"로 출력
        if constexpr (std::is_same_v<U, double>) {
            if (isAllZero(reinterpret_cast<const double*>(data), size)) {
                oss << type_name<T>() << " " << label << " = N/A;";
                info("{}", oss.str());
                return;
            }
        } else if constexpr (std::is_same_v<U, float>) {
            if (isAllZero(reinterpret_cast<const float*>(data), size)) {
                oss << type_name<T>() << " " << label << " = N/A;";
                info("{}", oss.str());
                return;
            }
        } else if constexpr (std::is_same_v<U, int>) {
            if (isAllZero(reinterpret_cast<const int*>(data), size)) {
                oss << type_name<T>() << " " << label << " = N/A;";
                info("{}", oss.str());
                return;
            }
        }

        // string 계열은 타입명 생략
        if constexpr (std::is_same_v<U, std::string> || std::is_same_v<U, const char*> || std::is_same_v<U, char*>) {
            oss << label << " = {";
        } else {
            oss << type_name<T>() << " " << label << " = {";
        }

        for (size_t i = 0; i < size; ++i) {
            oss << data[i];
            if (i + 1 < size) oss << ", ";
        }
        oss << "};";
        info("{}", oss.str());
    }

} // namespace logger


