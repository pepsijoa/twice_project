module;

import <iostream>
#include <unistd.h> // C++에서 read(), write()를 사용하기 위함
#include <string.h> // C++에서 strncmp()를 사용하기 위함

module moveController;

// 생성자
MoveController::MoveController(int serial_fd) 
    : serial_fd(serial_fd) 
{
    if (this->serial_fd < 0) {
        std::cerr << "MoveController: 시리얼 포트가 유효하지 않습니다!" << std::endl;
    }
}

// 소멸자
MoveController::~MoveController() {
    // (정리할 자원이 있다면 여기에 작성)
}

bool MoveController::processCommand(const std::string& msg) {
    if (serial_fd < 0) {
        std::cerr << "MoveController: 시리얼 포트가 유효하지 않습니다." << std::endl;
        return false; // 초기화 실패 상태
    }

    char command_to_arduino = '\0';

    // 웹 메시지 -> 아두이노 1바이트 명령으로 '번역'
    if (msg == "up")        command_to_arduino = 'F';
    else if (msg == "down") command_to_arduino = 'B';
    else if (msg == "left") command_to_arduino = 'L';
    else if (msg == "right") command_to_arduino = 'R';
    else {
        std::cout << "MoveController: 처리할 수 없는 명령 [" << msg << "]" << std::endl;
        return false;
    }

    // 아두이노로 명령을 보내고, "YES/NO/Error" 응답을 받음
    std::cout << "MoveController: 아두이노로 '" << command_to_arduino << "' 전송 및 응답 대기..." << std::endl;
    char response = sendCommandToArduino(command_to_arduino);

    // 응답을 mainController로 전달
    if (response == 'Y') {
        std::cout << "MoveController: 아두이노 응답 [YES]" << std::endl;
        return true;
    } else if (response == 'N') {
        std::cout << "MoveController: 아두이노 응답 [NO] (장애물)" << std::endl;
        return false;

    } else { 
        std::cout << "MoveController: 아두이노 통신 오류" << std::endl;
        return false;
    }
}

char MoveController::sendCommandToArduino(char cmd) {
    // moveController -> 아두이노로 1바이트 명령(F,B,L,R) 전송
    if (write(serial_fd, &cmd, 1) != 1) {
        std::cerr << "MoveController: 아두이노에 쓰기 실패" << std::endl;
        return 'E'; // Error
    }

    // 아두이노 -> moveController로부터 응답이 올 때까지 '대기(Block)'
    char read_buffer[64];
    ssize_t num_bytes = read(serial_fd, read_buffer, sizeof(read_buffer) - 1);

    if (num_bytes > 0) {
        read_buffer[num_bytes] = '\0'; // C-style 문자열로 만듦

        // 응답 파싱 ("YES\r\n" 또는 "NO\r\n") -> 아두이노에서 보내는 응답에 따라 수정해야됨
        if (strncmp(read_buffer, "YES", 3) == 0) {
            return 'Y';
        } else if (strncmp(read_buffer, "NO", 2) == 0) {
            return 'N';
        } else {
            std::cerr << "MoveController: 아두이노로부터 알 수 없는 응답: " << read_buffer << std::endl;
            return 'E';
        }
    } else {
        // 읽기 실패
        std::cerr << "MoveController: 아두이노로부터 읽기 실패" << std::endl;
        return 'E';
    }
}