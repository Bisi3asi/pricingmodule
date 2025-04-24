cmake_minimum_required(VERSION 3.16)
project(TestQuantLib)

set(CMAKE_CXX_STANDARD 17)

# Boost 라이브러리 찾기
set(BOOST_ROOT "C:/Program Files/boost_1_87_0")
set(Boost_INCLUDE_DIR ${BOOST_ROOT})
set(Boost_LIBRARY_DIR "${BOOST_ROOT}/lib")
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

# QuantLib 포함 및 링크 설정
set(QUANTLIB_ROOT "C:/Program Files/QuantLib")
include_directories(${QUANTLIB_ROOT})
link_directories("${QUANTLIB_ROOT}/lib")

# 테스트 실행파일 생성
add_executable(test main.cpp)

# 정적 라이브러리 링크
target_link_libraries(test QuantLib-x64-mt)