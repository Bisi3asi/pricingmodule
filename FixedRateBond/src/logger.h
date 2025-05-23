#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <string>
#include <vector>
#include <iomanip>
#include <iostream>
#include <filesystem>

void initLogger(const std::string& filename);

// Array 출력용
template <typename T>
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

// vector 출력용
template <typename T>
void logArrayLine(const std::string& label, const std::vector<T>& data, int precision = -1) {
    logArrayLine(label, data.data(), data.size(), precision);
}