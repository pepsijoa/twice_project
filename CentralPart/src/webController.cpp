module;

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

module webController;

// 생성자 구현
WebController::WebController(const char* socket_path) 
{
    unlink(socket_path); 

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "소켓 생성 실패" << std::endl;
        return;
    }

    // 소켓 옵션 설정 (재사용 가능)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "SO_REUSEADDR 설정 실패" << std::endl;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, socket_path, sizeof(server_addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "바인딩 실패" << std::endl;
        close(server_fd);
        server_fd = -1;
        return;
    }

    // listen backlog를 20으로 증가 (동시 연결 대기열 확대)
    if (listen(server_fd, 20) == -1) {
        std::cerr << "리스닝 실패" << std::endl;
        close(server_fd);
        server_fd = -1;
        return;
    }
    
    is_initialized = true;
    std::cout << "서버 시작: " << socket_path << std::endl;
}

// 소멸자 구현
WebController::~WebController() 
{
    if (client_fd != -1) {
        close(client_fd);
    }
    if (server_fd != -1) {
        close(server_fd);
    }
    std::cout << "서버 종료" << std::endl;
}

// is_ready 구현
bool WebController::is_ready() const 
{
    return is_initialized && server_fd != -1;
}

// accept_connection 구현
bool WebController::accept_connection() 
{
    if (!is_ready()) {
        std::cerr << "서버가 초기화되지 않았습니다" << std::endl;
        return false;
    }
    if (client_fd != -1) {
        close(client_fd);
        client_fd = -1;
    }
    client_fd = accept(server_fd, nullptr, nullptr);
    if (client_fd == -1) {
        std::cerr << "클라이언트 연결 실패" << std::endl;
        return false;
    }
    return true;
}

// receive_message 구현
bool WebController::receive_message(char* buffer, size_t buffer_size) 
{
    if (client_fd == -1) {
        std::cerr << "클라이언트가 연결되지 않았습니다" << std::endl;
        return false;
    }

    memset(buffer, 0, buffer_size); // 버퍼 초기화
    
    ssize_t num_bytes = recv(client_fd, buffer, buffer_size - 1, 0);
    
    if (num_bytes > 0) {
        buffer[num_bytes] = '\0'; // null-terminate
        std::cout << "받은 메시지: " << buffer << std::endl;
        return true;
    } 
    else if (num_bytes == 0) {
        std::cout << "클라이언트 연결 종료" << std::endl;
        close(client_fd);
        client_fd = -1;
        return false;
    } 
    else {
        std::cerr << "메시지 수신 실패" << std::endl;
        close(client_fd);
        client_fd = -1;
        return false;
    }
}

// send_response 구현
void WebController::send_response(const char* message)
{
    if (client_fd == -1) {
        std::cerr << "클라이언트가 연결되지 않았습니다" << std::endl;
        return;
    }

    ssize_t sent = send(client_fd, message, strlen(message), 0);
    if (sent == -1) {
        std::cerr << "응답 전송 실패" << std::endl;
    } 
    // else {
    //     std::cout << "응답 전송: " << message << std::endl;
    // }
}

