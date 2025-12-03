module;

#include <iostream>
#include <unistd.h> // C++에서 read(), write()를 사용하기 위함
#include <string.h> // C++에서 strncmp()를 사용하기 위함
#include <cstdint>
#include <termios.h>
#include <fcntl.h>
#include <cstring>

module moveController;

#pragma pack(push, 1) // 1 byte packing 을 통해 빈 공간 없이 데이터 받기 

/*
struct RobotDataPacket {
    float x;
    float y;
    float theta;
    uint8_t status;
};
*/

#pragma pack(pop)

// 생성자
MoveController::MoveController() : serial_fd(-1) {}

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
    options.c_cflag &= ~CRTSCTS; 
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
int MoveController::processCommand(const std::string& msg, const std::string& orientation, const int mappedIndex) {
    if (serial_fd < 0) {
        std::cerr << "MoveController: 시리얼 포트가 유효하지 않습니다." << std::endl;
        return -1; // 초기화 실패 상태
    }

    uint8_t command_to_arduino = 0;

    // 웹 메시지 -> 아두이노 1바이트 명령으로 '번역'
    if (msg == "up")        command_to_arduino = 0b11111000;
    else if (msg == "down") command_to_arduino = 0b11110100;
    else if (msg == "left") command_to_arduino = 0b11110010;
    else if (msg == "right") command_to_arduino = 0b11110001;
    
    else if (msg == "mapping_feature_arrive") {
        if(orientation == "up")        command_to_arduino = 0b00010001;
        else if (orientation == "down") command_to_arduino = 0b00010010;
        else if (orientation == "left") command_to_arduino = 0b00010100;
        else if (orientation == "right") command_to_arduino = 0b00011000;
        else{
            std::cerr << "MoveController: 잘못된 방향 정보 제공됨: " << orientation << std::endl;
            return -1;
        }
    }
    
    else if (msg == "mapped_feature_arrive") {
        //todo : mappedIndex는 15 이하의 값이어야 함
        command_to_arduino = (mappedIndex & 0b00001111);
    }
    else {
        std::cout << "MoveController: 처리할 수 없는 명령 [" << msg << "]" << std::endl;
        return -1;
    }

    int status = sendCommandToArduino(command_to_arduino);
    
    return status; // 0x02 -> Sensor 감지 0x01 -> 정상 이동 등 0x04 -> RX 에러
}

int MoveController::sendCommandToArduino(uint8_t cmd) {
    tcflush(serial_fd, TCIOFLUSH); // 송수신 버퍼 비우기
    if (write(serial_fd, &cmd, 1) != 1) {
        std::cerr << "MoveController: 아두이노에 쓰기 실패" << std::endl;
        return -1; // Error
    }

    uint8_t buffer[3] = {0, };
    int total_read = 0;
    int timeout_check = 0;
    
    while(total_read <3){
        int n = read(serial_fd, buffer + total_read, 3 - total_read);
        if(n > 0){
            total_read += n;
        }
        else{
            timeout_check++;
            usleep(1000); // 1ms 대기 (너무 빨리 돌면 CPU 낭비)
            if(timeout_check > 1000){ // 1초 이상 응답 없으면 타임아웃
                std::cerr << "MoveController: 아두이노로부터 응답 타임아웃" << std::endl;
                return -1;
            }
        }
    }

    if (buffer[0] == 0xAA && buffer[1] == 0xBB) {
        uint8_t status = buffer[2];
        std::cout << "RX: " << (int)status << std::endl;
        return (int)status; // 상태값 반환
    }
    std::cerr << "MoveController: 잘못된 응답 패킷" << std::endl;
    return -1; // Error
}

//     // 2. 응답 패킷 수신 (0xAA -> 0xBB -> Status)
//     uint8_t byte_in;
//     int attempt = 0;
    
//     // 최대 100번 시도 (타임아웃 방지)
//     while (attempt < 100) {
//         // 1바이트 읽기
//         if (read(serial_fd, &byte_in, 1) != 1) {
//             attempt++;
//             usleep(1000); // 1ms 대기 (너무 빨리 돌면 CPU 낭비)
//             continue;
//         }

//         // [Header 1] 0xAA 발견?
//         if (byte_in == 0xAA) {
//             // 그 다음 바이트 읽기
//             if (read(serial_fd, &byte_in, 1) == 1) {
//                 // [Header 2] 0xBB 발견?
//                 if (byte_in == 0xBB) {
                    
//                     // [Data] 마지막 3번째 바이트 (Status) 읽기
//                     uint8_t status_byte;
//                     if (read(serial_fd, &status_byte, 1) == 1) {
                        
//                         // 성공! 상태값 반환 (0x01, 0x02, 0x08 등)
//                         // std::cout << "Arduino Status: " << (int)status_byte << std::endl;
//                         return (int)status_byte; 
//                     }
//                 }
//             }
//         }
//         attempt++;
//     }
//     return -1; // 에러
// }

/*
RobotDataPacket packet;
uint8_t buffer[sizeof(RobotDataPacket)];

// read는 한 번에 다 못 읽을 수도 있으므로 루프로 처리하거나
// 간단하게 read 함수 호출 (여기선 간단히 처리)
int bytes_read = 0;
int total_bytes = sizeof(RobotDataPacket);

while(bytes_read < total_bytes) {
    int r = read(serial_fd, buffer + bytes_read, total_bytes - bytes_read);
    // read(int fd, void* buf, size_t nbytes); fd = 데이터 전송 대상, 수신 데이터 저장, 수신 최대 byte 수 
    if (r <= 0) break;
    bytes_read += r;
}

if (bytes_read == total_bytes) {
    // 버퍼를 구조체로 변환
    std::memcpy(&packet, buffer, sizeof(RobotDataPacket));

    return packet.status; // 상태값 반환
}*/