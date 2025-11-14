module;

#include <unistd.h>
#include <string.h> 
#include <iostream>
#include <cstdint>

export module moveController;
  

export class MoveController {
    public:
        // 생성자
        MoveController();

        // 소멸자
        ~MoveController();
        bool isReady() const;
        bool processCommand(const std::string& msg);

        bool openPort(const std::string& port_name, int baud_rate);
        void closePort();

    private:
        char sendCommandToArduino(uint8_t cmd);
        int serial_fd;      // 아두이노와 연결된 시리얼 포트의 파일 디스크립터
        int baud_rate;      // 통신 속도
};



