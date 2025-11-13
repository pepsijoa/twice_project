module;

#include <iostream>
#include <unistd.h> // C++에서 read(), write()를 사용하기 위함
#include <string.h> // C++에서 strncmp()를 사용하기 위함
#include <cstdint.h>
#include <termios.h>
#include <fcntl.h>

module moveController;

// 생성자
MoveController::MoveController() 
    : serial_fd(-1)

// 소멸자
MoveController::~MoveController() {
    closePort();
}

// 시리얼 포트 열기
bool MoveController::openPort(const std::string& port_name, int baud_rate) {
    if (isReady()) {
        std::cout << "MoveController: 이미 포트가 열려있습니다." << std::endl;
        return true;
    }

    serial_fd = open(port_name.c_str(), O_RDWR | O_NOCTTY);

    if (serial_fd == -1) {
        std::cerr << "MoveController: 시리얼 포트 열기 실패: " << port_name << std::endl;
        return false;
    }

    struct termios options;
    tcgetattr(serial_fd, &options); 

    speed_t baud;
    if (baud_rate == 9600) {
        baud = B9600;
    } else if (baud_rate == 115200) {
        baud = B115200;
    } else {
        std::cerr << "MoveController: 지원하지 않는 Baud Rate: " << baud_rate << std::endl;
        close(serial_fd);
        serial_fd = -1;
        return false;
    }
    cfsetispeed(&options, baud);
    cfsetospeed(&options, baud);


    options.c_cflag &= ~PARENB; 
    options.c_cflag &= ~CSTOPB; 
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag |= CREAD | CLOCAL;
    options.c_lflag &= ~ICANON; 
    options.c_lflag &= ~ECHO; 
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST; 
    options.c_cc[VMIN]  = 1;  // 1바이트 수신 대기
    options.c_cc[VTIME] = 50; // 5초 타임아웃

    if (tcsetattr(serial_fd, TCSANOW, &options) != 0) {
        std::cerr << "MoveController: 시리얼 포트 설정 적용 실패" << std::endl;
        close(serial_fd);
        serial_fd = -1;
        return false;
    }

    std::cout << "MoveController: 시리얼 포트 " << port_name << " (" << baud_rate << "bps) 열기 성공." << std::endl;
    return true;
}

// 시리얼 포트 닫기
void MoveController::closePort() {
    if (isReady()) {
        close(serial_fd);
        serial_fd = -1;
        std::cout << "MoveController: 시리얼 포트 닫힘." << std::endl;
    }
}

// 시리얼 포트가 열려있는지 확인
bool MoveController::isReady() const {
    return serial_fd >= 0;
}

// 명령 처리
bool MoveController::processCommand(const std::string& msg) {
    if (serial_fd < 0) {
        std::cerr << "MoveController: 시리얼 포트가 유효하지 않습니다." << std::endl;
        return false; // 초기화 실패 상태
    }

    uint8_t command_to_arduino = 0;

    // 웹 메시지 -> 아두이노 1바이트 명령으로 '번역'
    if (msg == "up")        command_to_arduino = 0b11111000;
    else if (msg == "down") command_to_arduino = 0b11110100;
    else if (msg == "left") command_to_arduino = 0b11110010;
    else if (msg == "right") command_to_arduino = 0b11110001;
    else {
        std::cout << "MoveController: 처리할 수 없는 명령 [" << msg << "]" << std::endl;
        return false;
    }

    // 아두이노로 명령을 보내고, "YES/NO/Error" 응답을 받음
    //std::cout << "MoveController: 아두이노로 '" << command_to_arduino << "' 전송 및 응답 대기..." << std::endl;
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

char MoveController::sendCommandToArduino(uint8_t cmd) {
    if (write(serial_fd, &cmd, 1) != 1) {
        std::cerr << "MoveController: 아두이노에 쓰기 실패" << std::endl;
        return 'E'; // Error
    }

    uint8_t response_auduino;
    ssize_t num_bytes = read(serial_fd, &response_auduino, 1);

    if (num_bytes == 1) {

        if (response_auduino == 0b00000001) {
            return 'Y';
        } else if (response_auduino == 0b00000010) {
            return 'N';
        } else {
            std::cerr << "MoveController: 아두이노로부터 알 수 없는 응답: " << std::endl;
            return 'E';
        }
    } else {
        // 읽기 실패
        std::cerr << "MoveController: 아두이노로부터 읽기 실패" << std::endl;
        return 'E';
    }
}