#pragma once

#include "common.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/fmt/bundled/core.h>
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
            oss << body << " (" << file << ":" << line << ")";
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
    void logArrayLine(const std::string& label, const T* data, size_t size, int precision = 10);

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
    // Format: (trailing-zero compressed): <타입> <배열/벡터명>[] = {val1, val2, ..., 0, ... , 0};
    template <typename T>
    void logArrayLine(const std::string& label, const T* data, size_t size, int precision) {
        std::ostringstream oss;
        if (precision >= 0)
            oss.setf(std::ios::fixed), oss.precision(precision);
        using U = std::remove_cv_t<std::remove_reference_t<T>>;

        // case 1. 부동소수점 배열 처리: 모든 원소가 0이면 N/A로 출력.
        // find_last_nonzero 헬퍼를 사용하여 trailing-zero 구간을 압축해서 표시.
        if constexpr (std::is_floating_point_v<U>) { // 
            size_t last_nonzero = find_last_nonzero(reinterpret_cast<const U*>(data), size);
            if (last_nonzero == SIZE_MAX) {
                oss << type_name<T>() << " " << label << " = N/A;";
                info("{}", oss.str());
                return;
            }
            size_t trailing_start = last_nonzero + 1;

            // 소수점 자리수 고정을 위한 람다 포맷터
            auto format_val = [&](auto v) -> std::string {
                if (precision >= 0) return fmt::format("{:.{}f}", v, precision);
                return fmt::format("{}", v);
            };
            // 람다 포맷터로 소수점 자리수 고정된 0 문자열 미리 준비
            std::string zeroStr = format_val(static_cast<U>(0));
            // <type> <label> 헤더 준비
            oss << type_name<T>() << " " << label << " = {";
            // 처음부터 last_nonzero 까지 출력
            for (size_t i = 0; i <= last_nonzero; ++i) {
                oss << format_val(data[i]);
                if (i < last_nonzero) oss << ", ";
            }

            // trailing-zero 구간이 있으면 압축 출력
            if (trailing_start < size) {
                size_t trailing_count = size - trailing_start;
                if (last_nonzero < size) oss << ", ";
                if (trailing_count == 1) {
                    oss << zeroStr;
                } else {
                    oss << zeroStr << ", ... , " << zeroStr;
                }
            }
            oss << "};";
            info("{}", oss.str());
            return;

        // case 2. 정수형 배열에 대해서는 전체가 0인 경우의 N/A 처리나 별도 0 ... 0 trailing 비활성화
        } else if constexpr (std::is_integral_v<U>) {
            // int, long, short
        }

        // case 3. string 계열은 타입명 생략
        if constexpr (std::is_same_v<U, std::string> || std::is_same_v<U, const char*> || std::is_same_v<U, char*>) {
            oss << label << " = {";
        } else {
            oss << type_name<T>() << " " << label << " = {";
        }

        // fallback. 모든 배열의 원소를 출력
        for (size_t i = 0; i < size; ++i) {
            oss << data[i];
            if (i + 1 < size) oss << ", ";
        }
        oss << "};";
        info("{}", oss.str());
    }
} // namespace logger


