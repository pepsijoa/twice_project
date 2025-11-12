module;

#include <unistd.h>
#include <string.h> 
#include <iostream>

export module moveController;
  

export class MoveController {
    public:
        // 생성자
        MoveController(int serial_fd, int baud_rate);

        // 소멸자
        ~MoveController();
        bool isRead();
        bool processCommand(const std::string& msg);

    private:
        char sendCommandToArduino(char cmd);
        int serial_fd = -1;      // 아두이노와 연결된 시리얼 포트의 파일 디스크립터
        int baud_rate = 9600;      // 통신 속도
};



