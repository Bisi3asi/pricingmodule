# CMake 최소 버전
cmake_minimum_required(VERSION 3.13)

#-----------------------------------------------------------------------------------------
# 1. [동적 라이브러리 생성]
## 프로젝트 이름
project(OtStock)

## cpp, h 등 소스 파일 지정(동적 라이브러리 생성용)
file(GLOB SOURCE_FILES "src/*.cpp") # 

## 동적 라이브러리 파일 생성
add_library(OtStock SHARED ${SOURCE_FILES})

## lib 접두사 제거 (Linux 환경 .so 파일 default 접두사)
set(CMAKE_SHARED_LIBRARY_PREFIX "")

## 출력 파일 이름 지정
set_target_properties(OtStock PROPERTIES OUTPUT_NAME "OtStock")
#-----------------------------------------------------------------------------------------

#-----------------------------------------------------------------------------------------
# 2. [테스트 및 디버그용 실행 파일 생성]
## 소스 파일 지정(실행 파일 생성용)
set(TEST_EXEC_FILES main.cpp)

## 실행 파일 생성
add_executable(main ${TEST_EXEC_FILES})

## 실행 파일 링크할 동적 라이브러리 경로 지정
link_directories(${CMAKE_BINARY_DIR}/lib)

## 실행 파일과 동적 라이브러리 링크
target_link_libraries(main PRIVATE OtStock)

# (linux) 실행 파일 및 동적라이브러리 출력 디렉토리 정의
## CMAKE_BINARY_DIR은 CMakePresets.json에서 설정 변경 가능
set_target_properties(OtStock PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
#-----------------------------------------------------------------------------------------
