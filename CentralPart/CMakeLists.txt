# 최소 CMake 버전 요구사항
cmake_minimum_required(VERSION 3.20)

# 프로젝트 이름과 C++ 버전 설정
project(CentralPartProject LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# 아키텍처 감지
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "64-bit 아키텍처 감지됨")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(STATUS "32-bit 아키텍처 감지됨")
else()
    message(WARNING "알 수 없는 아키텍처")
endif()

# 프로세서 아키텍처 출력
message(STATUS "시스템 프로세서: ${CMAKE_SYSTEM_PROCESSOR}")

# UTF-8 인코딩 설정
add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)

# OpenCV 찾기 (GStreamer 지원 포함)
find_package(OpenCV 4 REQUIRED)
message(STATUS "OpenCV 버전: ${OpenCV_VERSION}")
message(STATUS "OpenCV 라이브러리: ${OpenCV_LIBS}")

# GStreamer 찾기 (libcamera 지원용)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0)

set(PROJECT_NAME twiceproject)

# 모듈 파일 추가
add_library(modules)
target_sources(modules
    PUBLIC
        FILE_SET CXX_MODULES FILES
        src/mainController.cppm
        src/webController.cppm
        src/mapper.cppm
        src/camController.cppm
        src/moveController.cppm
    PRIVATE
        src/mainController.cpp
        src/webController.cpp
        src/mapper.cpp
        src/camController.cpp
        src/moveController.cpp
)
target_include_directories(modules PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 실행 파일 추가
add_executable(${PROJECT_NAME}
    src/main.cpp
)

# 모듈 라이브러리 링크
target_link_libraries(${PROJECT_NAME} PRIVATE 
    modules 
    ${OpenCV_LIBS}
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_VIDEO_LIBRARIES}
)

# OpenCV 및 GStreamer include 디렉토리 설정
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_VIDEO_INCLUDE_DIRS}
)

# 모듈에도 OpenCV 및 GStreamer 링크
target_link_libraries(modules PRIVATE
    ${OpenCV_LIBS}
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_VIDEO_LIBRARIES}
)

target_include_directories(modules PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_VIDEO_INCLUDE_DIRS}
)