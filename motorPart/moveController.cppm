module;

#include <unistd.h>
#include <string.h> 

export module moveController;

import <string>;    
import <iostream>;  
import mapper;      

export class MoveController {
public:
    // 생성자
    MoveController(Mapper* mapper, int serial_fd);

    // 소멸자
    ~MoveController();

    bool processCommand(const std::string& msg);

private:
    char sendCommandToArduino(char cmd);

    int serial_fd;      // 아두이노와 연결된 시리얼 포트의 파일 디스크립터
};



