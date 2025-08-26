#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <string>
#include <vector>
#include <iomanip>
#include <iostream>
#include <filesystem>

/* 로거 초기화 및 생성 함수 */
void disableConsoleLogging();
void initLogger(const std::string& filename);


/* 로깅용 템플릿 함수 */
template <typename T> // Array 출력용
void logArrayLine(const std::string& label, const T* data, size_t size, int precision = -1) {
    std::ostringstream oss;
    if (precision >= 0) {
        oss << std::fixed << std::setprecision(precision);
    }
    oss << label << ": ";
    for (size_t i = 0; i < size; ++i) {
        oss << data[i];
        if (i != size - 1) {
            oss << ", ";
        }
    }
    spdlog::info("{}", oss.str());
}

template <typename T> // vector 출력용
void logArrayLine(const std::string& label, const std::vector<T>& data, int precision = -1) {
    logArrayLine(label, data.data(), data.size(), precision);
}