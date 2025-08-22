cmake_minimum_required(VERSION 3.16)
project(Root)

# 각 모듈별 서브디렉토리 루트 추가
add_subdirectory(CommonUtils)
add_subdirectory(Bond)
add_subdirectory(Leg)
add_subdirectory(OtStock)
add_subdirectory(Net)