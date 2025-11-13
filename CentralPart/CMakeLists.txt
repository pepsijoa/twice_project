# 최소 CMake 버전 요구사항
cmake_minimum_required(VERSION 3.20)

# 프로젝트 이름과 C++ 버전 설정
project(CentralPartProject LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# UTF-8 인코딩 설정
add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)

set(PROJECT_NAME twiceproject)

# 모든 소스 파일 추가
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/mainController.cpp
    src/webController.cpp
    src/mapper.cpp
    src/camController.cpp
    src/moveController.cpp
)

# include 디렉토리 설정
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)