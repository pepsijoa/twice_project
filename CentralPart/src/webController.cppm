module;

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

export module webController;

// 인터페이스 선언 (헤더 역할)
export class WebController {
    private:
        int server_fd = -1;
        int client_fd = -1;
        sockaddr_un server_addr{};
        bool is_initialized = false;

    public:
        // 생성자
        WebController(const char* socket_path);
        
        // 소멸자
        ~WebController();

        // 서버 초기화 확인
        bool is_ready() const;

        // 클라이언트 연결 수락
        bool accept_connection();

        // 메시지 수신
        bool receive_message(char* buffer, size_t buffer_size);

        // 응답 전송
        void send_response(const char* message);
};