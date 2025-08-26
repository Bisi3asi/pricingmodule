cmake_minimum_required(VERSION 3.20)

# [모듈별 개별 설정 내용]
# =========================================================================
project(Leg) # 모듈명 (대문자/소문자 구분)
set(TEST_EXEC_NAME "test_leg") # 테스트 실행 파일을 지정할 .cpp 파일명
set(OUTPUT_LIBRARY_NAME "leg") # 출력 라이브러리 파일명 지정
# =========================================================================

# 1. 소스 수집 및 정적 라이브러리 생성
file(GLOB SOURCE_FILES "src/*.cpp")
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

# 2. 타겟 속성 설정
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${OUTPUT_LIBRARY_NAME} 	# 출력 라이브러리 파일명 설정
    PREFIX "" 	# Linux .so 파일 생성 시 lib 접두사 제거
	POSITION_INDEPENDENT_CODE ON
)

# (Windows) function 외부 노출
if (WIN32) 
	set(BUILD_LIBRARY ON)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_LIBRARY)
endif()

# 3. 의존성 라이브러리 연결
target_link_libraries(${PROJECT_NAME} 
	PUBLIC CommonUtils QuantLib::QuantLib 
	PRIVATE Boost::system Boost::filesystem
)

# 3-1. Linux C++17 filesystem 사용 시 (GCC 9.1 미만 버전에서만 필요)
if(UNIX AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.1")
    target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs)
endif()

# 4. 테스트 실행 파일 생성
add_executable(${TEST_EXEC_NAME} "${TEST_EXEC_NAME}.cpp") # 실행 파일 생성 .cpp -> .exe
target_link_libraries(${TEST_EXEC_NAME} PRIVATE ${PROJECT_NAME}) # 실행 파일 - 동적 라이브러리 링크

# 5. 출력 디렉토리 설정 (주석 해제 시 출력 디렉토리 변경됨)
# set_target_properties(${PROJECT_NAME} PROPERTIES
#     ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
#     LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
#     RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
# )

if (UNIX) # 리눅스의 경우 RPATH로 so 파일 경로 탐색
	set_target_properties(${TEST_EXEC_NAME} PROPERTIES BUILD_RPATH ${CMAKE_BINARY_DIR}) 
endif()

#6. (Linux) so 파일 용량 최적화 - 디버깅 심볼 제거
if(UNIX AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND strip --strip-all $<TARGET_FILE:${PROJECT_NAME}>
        COMMENT "Stripping debug symbols from the library"
    )
endif()