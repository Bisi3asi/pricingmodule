# CMake 최소 버전
cmake_minimum_required(VERSION 3.8)

# 1. [동적 라이브러리 생성]
## 프로젝트 이름
project(MakeDll)

## 소스 파일 지정(동적 라이브러리 생성용, 단건)
set(SOURCE_FILES "MyMath.c")

## 동적 라이브러리 파일 생성
add_library(MakeDll SHARED ${SOURCE_FILES})

## lib 접두사 제거 (Linux 환경 .so 파일 default 접두사)
#set(CMAKE_SHARED_LIBRARY_PREFIX "")

## 출력 파일 이름 지정
set_target_properties(PROPERTIES OUTPUT_NAME "MakeDll")

# 라이브러리 출력 디렉토리 설정 (선택 사항)
## CMAKE_BINARY_DIR은 CMakePresets.json에서 설정 변경 가능
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# 2. [테스트 및 디버그용 실행 파일 생성]
## 소스 파일 지정(실행 파일 생성용)
set(TEST_EXEC_FILES "Main.c")

## 실행 파일 생성
add_executable(main ${TEST_EXEC_FILES})

## 동적 라이브러리와 실행 파일을 링크
target_link_libraries(main PRIVATE MakeDll)

## 링크 라이브러리 경로
link_directories(${CMAKE_BINARY_DIR}/lib)