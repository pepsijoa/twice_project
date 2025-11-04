# 최소 CMake 버전 요구사항 (모듈 지원을 위해 3.28 이상 권장)
cmake_minimum_required(VERSION 3.28)

# 프로젝트 이름과 C++ 버전 설정
project(CentralPartProject LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_NAME twiceproject)
add_executable(${PROJECT_NAME})

# C++20 모듈 파일 추가 (*.cppm 파일만)
target_sources(${PROJECT_NAME}
  PUBLIC
    FILE_SET CXX_MODULES FILES
      src/webController.cppm
      src/mainController.cppm
)

# 일반 소스 파일 추가 (*.cpp 파일)
target_sources(${PROJECT_NAME}
  PRIVATE
    src/mainController.cpp
    src/webController.cpp
    src/main.cpp
)