##################################### 기본 프로젝트 설정 #####################################
cmake_minimum_required(VERSION 3.16)
project(FxForward LANGUAGES CXX)	# 1. 프로젝트명 설정
set(TEST_EXEC_NAME "main")			# 2. 테스트용 cpp 파일, 테스트 실행 파일명 설정 (기본값 main.cpp, main.exe)
set(OUTPUT_FILE_NAME "fxForward")	# 3. 출력 라이브러리 파일명 설정 (.dll, .so)
##############################################################################################

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 0. 컴파일러 최적화
if (WIN32)
	set_property(GLOBAL PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
elseif (UNIX)
	set(CMAKE_BUILD_TYPE Release)	# 디버깅 정보 미포함, 용량 최적화
	set(CMAKE_CXX_FLAGS_RELEASE "-g0 -O3")	
	add_compile_options(-ffunction-sections -fdata-sections)
	add_link_options(-Wl,--gc-sections)
endif()

# 1. 플랫폼별 Boost, QuantLib 환경변수, 컴파일러 세팅
if (WIN32)	
	message(STATUS "Configuring for Windows")

	add_compile_options("/MT")										# 정적 컴파일 설정
	set(Boost_INCLUDE_DIR "C:/Program Files/boost_1_87_0")			# Boost 로컬 설치 경로
	set(QUANTLIB_ROOT "C:/Program Files/QuantLib")					# QuantLib 로컬 설치 경로
	set(QUANTLIB_LIB_NAME "QuantLib-x64-mt-s")						# QuantLib 라이브러리명 (Linux ↔ Windows 상이)

elseif (UNIX)
	message(STATUS "Configuring for Linux")

	add_compile_options(-static-libgcc -static-libstdc++)			
	set(Boost_INCLUDE_DIR "/usr/lib/boost_1_87_0")
	set(QUANTLIB_ROOT "/usr/lib/QuantLib")
	set(QUANTLIB_LIB_NAME "QuantLib.a")
endif()

# 2. Boost, spdlog 라이브러리 연결 
include_directories(${Boost_INCLUDE_DIR})							# Boost_INCLUDE_DIR 설정
include_directories(${CMAKE_SOURCE_DIR}/include)					# 라이브러리 경로 설정
set(Boost_USE_STATIC_LIBS ON) # 정적 링크 강제
find_package(Boost REQUIRED COMPONENTS system filesystem)			# Boost 라이브러리 탐색

# 3. 동적 라이브러리 FxForward 생성
file(GLOB SOURCE_FILES "src/*.cpp")	# 소스 파일 수집
add_library(${CMAKE_PROJECT_NAME} SHARED ${SOURCE_FILES})	# 동적 라이브러리 생성 정의
set(CMAKE_SHARED_LIBRARY_PREFIX "")	# (Linux) 라이브러리 파일 생성 시 lib 접두사 제거
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${OUTPUT_FILE_NAME}) # 출력 파일명 정의

# (Windows) pricing function 외부 노출
if (WIN32) 
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE BUILD_LIBRARY) 
endif()

# 4. QuantLib - 동적 라이브러리 링크
include_directories(${QUANTLIB_ROOT})
link_directories("${QUANTLIB_ROOT}/lib")
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE "${QUANTLIB_ROOT}/lib")
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${QUANTLIB_LIB_NAME})

# 5. 테스트 실행 파일 main 생성
add_executable(${TEST_EXEC_NAME} "${TEST_EXEC_NAME}.cpp")	# 실행 파일 생성 .cpp -> .exe
target_link_libraries(${TEST_EXEC_NAME} PRIVATE ${CMAKE_PROJECT_NAME})	# 실행 파일 - 출력 동적 라이브러리 링크

# 6. 출력 디렉토리 설정
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if (UNIX)
	set_target_properties(${TEST_EXEC_NAME} PROPERTIES BUILD_RPATH ${CMAKE_BINARY_DIR})	# 리눅스의 경우 RPATH로 so 파일 경로 탐색
endif()

#7. (Linux) so 파일 용량 최적화 - 디버깅 심볼 제거
if(UNIX)
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND strip --strip-all $<TARGET_FILE:${OUTPUT_FILE_NAME}>
        COMMENT "Stripping debug symbols from the library"
    )
endif()