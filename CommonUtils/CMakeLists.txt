##################################### 기본 프로젝트 설정 #####################################
cmake_minimum_required(VERSION 3.16)
project(CommonUtils LANGUAGES CXX)  # 프로젝트명 설정
set(OUTPUT_FILE_NAME "commonUtils") # 출력 라이브러리 파일명 설정
set(MODULE_VERSION "1.0.0") 	    # 모듈 개발 버전 설정    
##############################################################################################

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 0. 플랫폼별 빌드 설정
if (WIN32)
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>") # MSVC UTF-8 인코딩 설정 (for spdlog logging library)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_property(GLOBAL PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDebug") # 정적 컴파일 설정 (for debug)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi") # Debug 최적화 비활성화, 디버깅 심볼 생성
    elseif (CMAKE_BUILD_TYPE STREQUAL "Release") 
        set_property(GLOBAL PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded") # 정적 컴파일 설정 (for release)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2") # Release 최적화, 디버깅 심볼 미생성
    endif()
elseif (UNIX)
    set(CMAKE_CXX_FLAGS_RELEASE "-g0 -O3") # 디버깅 정보 미포함, 용량 최적화
    add_compile_options(-ffunction-sections -fdata-sections)
    add_link_options(-Wl,--gc-sections)

    # src, include 소스 원격지 복사 (향후 개별 모듈 컴파일 간 필요)
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/include" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}) # spdlog 헤더 파일 복사
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/src" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}) # 유틸 헤더 파일 복사
endif()

# 1. 플랫폼별 Boost, QuantLib 설정
if (WIN32)
    message(STATUS "Configuring for Windows")   
    set(Boost_INCLUDE_DIR "C:/Program Files/boost_1_87_0") # Boost 로컬 설치 경로
    set(QUANTLIB_ROOT "C:/Program Files/QuantLib") # QuantLib 로컬 설치 경로

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options("/MTd") # 정적 컴파일 설정 (for debug)
        set(QUANTLIB_LIB_NAME "QuantLib-x64-mt-sgd") # QuantLib .lib 파일 (for debug)
    elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options("/MT") # 정적 컴파일 설정 (for release)
        set(QUANTLIB_LIB_NAME "QuantLib-x64-mt-s") # QuantLib .lib 파일 (for release)
    endif()
elseif (UNIX)
    message(STATUS "Configuring for Linux")
    add_compile_options(-static-libgcc -static-libstdc++)
    set(Boost_INCLUDE_DIR "/usr/lib/boost_1_87_0") # Boost 로컬 설치 경로
    set(QUANTLIB_ROOT "/usr/lib/QuantLib") # QuantLib 로컬 설치 경로
    set(QUANTLIB_LIB_NAME "QuantLib.a") # QuantLib .a 파일 (for linux)
endif()

# 2. Boost, spdlog, QuantLib 헤더 파일 경로 설정
include_directories(${Boost_INCLUDE_DIR}) # Boost                         
include_directories(${CMAKE_SOURCE_DIR}/include) # spdlog 
include_directories(${QUANTLIB_ROOT}) # QuantLib

# 3. 소스 수집 및 정적 라이브러리 생성
file(GLOB SOURCE_FILES "src/*.cpp")                               
add_library(${CMAKE_PROJECT_NAME} STATIC ${SOURCE_FILES})
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES 
    OUTPUT_NAME ${OUTPUT_FILE_NAME}
    VERSION ${MODULE_VERSION}
    POSITION_INDEPENDENT_CODE ON # Linux -fPIC 설정, Windows에서는 영향 없음
)

# 4. spdlog 라이브러리 연결
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include) #include할 타겟이 헤더 탐색 가능

# 5. Boost, QuantLib 라이브러리 연결
find_package(Boost REQUIRED COMPONENTS system filesystem)
link_directories("${QUANTLIB_ROOT}/lib")
target_link_libraries(${CMAKE_PROJECT_NAME} 
    PRIVATE ${QUANTLIB_LIB_NAME} Boost::system Boost::filesystem
)

# 6. 출력 디렉토리 설정
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} 
)
