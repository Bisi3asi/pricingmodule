cmake_minimum_required(VERSION 3.20)
project(Root LANGUAGES CXX)

# Cmake 기본 설정
cmake_policy(SET CMP0144 NEW) # find_package가 <PackageName>_ROOT 변수를 사용하도록 설정
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 의존성 라이브러리 탐색 및 설정 (from CMakePresets.json)
set(Boost_USE_STATIC_LIBS ON) # Boost 정적 링크 강제
find_package(Boost REQUIRED COMPONENTS system filesystem)
find_package(QuantLib REQUIRED)

message(STATUS "Found Boost version: ${Boost_VERSION_STRING}")
message(STATUS "Found QuantLib version: ${QuantLib_VERSION}")

# 플랫폼별 컴파일 관련 설정
if (WIN32)
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>") # MSVC UTF-8 인코딩 설정 (for spdlog logging library)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options("/MTd") # 정적 컴파일 설정 (for debug)
    else()
        add_compile_options("/MT") # 정적 컴파일 설정 (for release)
    endif()
elseif(UNIX)
    message(STATUS "Configuring for Unix/Linux")
    add_compile_options(-static-libgcc -static-libstdc++) # 정적 컴파일 설정 (for linux gcc)
    add_link_options(-Wl,--gc-sections) # 사용하지 않는 함수/데이터 제거 설정(용량 최적화)
endif()

# build all 관련
# 서브디렉토리에서 이 function을 호출하는 경우 해당 모듈의 실행 파일을 전역 목록에 등록
function(register_for_build_all tgt)
    set_property(GLOBAL APPEND PROPERTY PROJECT_BUILD_ALL_TARGETS "${tgt}")
endfunction()

# 하위 모듈 프로젝트 추가
add_subdirectory(CommonUtils)
add_subdirectory(Bond)
add_subdirectory(Leg)
add_subdirectory(OtStock)
add_subdirectory(Net)

# build all 관련
# 앞서 전역 디렉토리로 모은 모든 서브 실행파일 목록을 읽어 build_all target에 연결
get_property(_proj_targets GLOBAL PROPERTY PROJECT_BUILD_ALL_TARGETS)
add_custom_target(build_all DEPENDS
    CommonUtils
    Bond
    Leg
    OtStock
    Net
)
if(_proj_targets)
        add_dependencies(build_all ${_proj_targets})
endif()