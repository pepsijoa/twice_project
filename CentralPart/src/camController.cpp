module;

#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <utility>
#include <algorithm>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

module camController;

CamController::CamController() 
{
    std::cout << "카메라 컨트롤러 생성" << std::endl;
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, socket_path);
}

bool CamController::init(int max_retries, int retry_delay_ms)
{
    std::cout << "[C++] 카메라 서버 연결 시도 (최대 " << max_retries << "회)..." << std::endl;
    
    for (int attempt = 1; attempt <= max_retries; ++attempt) {
        if (connectToServer()) {
            std::cout << "[C++] ✅ Python 카메라 서버 연결 완료 (" << attempt << "/" << max_retries << ")" << std::endl;
            return true;
        }
        
        if (attempt < max_retries) {
            std::cout << "[C++] 재시도 중... (" << attempt << "/" << max_retries << ")" << std::endl;
            usleep(retry_delay_ms * 2000);
        }
    }
    
    std::cerr << "[C++] ❌ 카메라 서버 연결 실패 (최대 재시도 횟수 초과)" << std::endl;
    return false;
}

bool CamController::connectToServer()
{
    // 이미 연결되어 있으면 재연결 시도
    if (sock >= 0) {
        close(sock);
        sock = -1;
    }
    
    // 소켓 생성
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "[C++] 소켓 생성 실패" << std::endl;
        return false;
    }

    // Python 서버에 연결
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock);
        sock = -1;
        return false;
    }
    
    return true;
}

bool CamController::isConnected() const
{
    return sock >= 0;
}

std::string CamController::getArucoDataRaw()
{
    if (!isConnected()) {
        std::cerr << "[C++] 소켓이 연결되지 않음" << std::endl;
        return "{}";
    }
    
    std::string msg = "get_marker_data\n";
    ssize_t sent = send(sock, msg.c_str(), msg.length(), 0);
    
    if (sent < 0) {
        std::cerr << "[C++] 데이터 전송 실패" << std::endl;
        return "{}";
    }

    // JSON 데이터 수신 (개행 문자까지 읽기)
    std::string response;
    char ch;
    while (read(sock, &ch, 1) > 0) {
        if (ch == '\n') break;
        response += ch;
    }
    
    if (response.empty()) {
        std::cerr << "[C++] 응답 없음" << std::endl;
        return "{}";
    }
    
    return response;
}

std::vector<std::pair<int,int>> CamController::updateInventory()
{
    std::vector<std::pair<int,int>> inventoryDatas;
    
    // JSON 문자열 가져오기
    std::string json_str = getArucoDataRaw();
    
    // 수동 파싱: {"1": 2, "3": 1} 형태
    if (json_str == "{}") {
        return inventoryDatas; // 빈 데이터
    }
    
    // 중괄호 제거
    size_t start = json_str.find('{');
    size_t end = json_str.find('}');
    if (start == std::string::npos || end == std::string::npos) {
        std::cerr << "[C++] JSON 형식 오류" << std::endl;
        return inventoryDatas;
    }
    
    std::string content = json_str.substr(start + 1, end - start - 1);
    if (content.empty()) {
        return inventoryDatas;
    }
    
    // 쉼표로 분리하여 각 항목 파싱
    std::stringstream ss(content);
    std::string item;
    
    while (std::getline(ss, item, ',')) {
        // "1": 2 형태를 파싱
        size_t colon_pos = item.find(':');
        if (colon_pos == std::string::npos) continue;
        
        std::string key_part = item.substr(0, colon_pos);
        std::string value_part = item.substr(colon_pos + 1);
        
        // 따옴표와 공백 제거
        key_part.erase(std::remove(key_part.begin(), key_part.end(), '"'), key_part.end());
        key_part.erase(std::remove(key_part.begin(), key_part.end(), ' '), key_part.end());
        value_part.erase(std::remove(value_part.begin(), value_part.end(), ' '), value_part.end());
        
        try {
            int id = std::stoi(key_part);
            int count = std::stoi(value_part);
            
            std::cout << "  -> 마커 ID: " << id << ", 개수: " << count << std::endl;
            inventoryDatas.push_back({id, count});
        } catch (const std::exception& e) {
            std::cerr << "[C++] 파싱 에러: " << e.what() << std::endl;
        }
    }
    
    return inventoryDatas;
}
CamController::~CamController()
{
    std::cout << "카메라 컨트롤러 소멸" << std::endl;
    if (sock >= 0) {
        // Python 서버에 종료 신호 전송
        std::string quit_msg = "quit\n";
        send(sock, quit_msg.c_str(), quit_msg.length(), 0);
        close(sock);
    }
}