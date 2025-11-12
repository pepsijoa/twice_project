# 최소 CMake 버전 요구사항 (모듈 지원을 위해 3.28 이상 권장)
cmake_minimum_required(VERSION 3.28)

# 프로젝트 이름과 C++ 버전 설정
project(TwiceProject LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 실행 파일 생성 (이름은 'TwiceProject'가 됩니다)
add_executable(${PROJECT_NAME})

# 2. C++20 모듈 인터페이스 파일 추가 (*.cppm)
target_sources(${PROJECT_NAME}
  PUBLIC
    FILE_SET CXX_MODULES FILES
      CentralPart/src/webController.cppm
      CentralPart/src/mainController.cppm
      CentralPart/src/mapper.cppm
      CentralPart/src/camController.cppm
      motorPart/moveController.cppm 
)

# 3. 일반 소스 파일 추가 (*.cpp)
target_sources(${PROJECT_NAME}
  PRIVATE
    CentralPart/src/main.cpp           
    CentralPart/src/mainController.cpp
    CentralPart/src/webController.cpp
    CentralPart/src/mapper.cpp
    CentralPart/src/camController.cpp
    motorPart/moveController.cpp    
)

# 4.스레드 라이브러리 링크
# std::thread (mainController)를 사용하려면 pthread 라이브러리가 필요합니다.
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)