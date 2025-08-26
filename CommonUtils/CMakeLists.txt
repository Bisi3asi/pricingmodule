cmake_minimum_required(VERSION 3.20)

project(CommonUtils) # 모듈명 (대문자/소문자 구분)

# 1. 소스 수집 및 정적 라이브러리 생성
file(GLOB SOURCE_FILES "src/*.cpp")
add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

# 2. 타겟 속성 설정
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "commonUtils" # 출력 라이브러리 파일명 설정
    POSITION_INDEPENDENT_CODE ON
)

# 3. 하위 모듈 적용을 위한 헤더 공개 경로 지정)
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src> # commonUtils/src
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> # commonUtils/include (spdlog)
)

# 4. 의존성 라이브러리 연결
target_link_libraries(${PROJECT_NAME}
    PUBLIC QuantLib::QuantLib
    PRIVATE Boost::system Boost::filesystem
)

# 4-1. Linux C++17 filesystem 사용 시 (GCC 9.1 미만 버전에서만 필요)
if(UNIX AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.1")
    target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs)
endif()

# 5. 출력 디렉토리 설정 (주석 해제 시 출력 디렉토리 변경됨)
# set_target_properties(${PROJECT_NAME} PROPERTIES
#     ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
# )

# 6. Linux 환경에서 소스 원격지 복사
if(UNIX AND NOT WIN32)
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/include" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/src" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
