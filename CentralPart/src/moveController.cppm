module;

#include <unistd.h>
#include <string>
#include <iostream>
#include <cstdint>
#include <memory>

export module moveController;
  

export class MoveController {
    public:
        // 생성자
        MoveController();

        // 소멸자
        ~MoveController();
        bool isReady() const;
        int processCommand(const std::string& msg, const std::string& orientation = "", const int mappedIndex = -1);

        bool openPort(const std::string& port_name, int baud_rate);
        void closePort();

    private:
        int sendCommandToArduino(uint8_t cmd);
        int serial_fd;      // 아두이노와 연결된 시리얼 포트의 파일 디스크립터
        int baud_rate;      // 통신 속도
};



