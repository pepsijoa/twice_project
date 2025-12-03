module;

#include <iostream>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <nlohmann/json.hpp> 

using json = nlohmann::json;

module camController;

CamController::CamController() 
{
    std::cout << "카메라 컨트롤러 생성" << std::endl;
    
    // 소켓 생성
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "[C++] 소켓 생성 실패" << std::endl;
        sock = -1;
        return;
    }

    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, socket_path);

    // Python 서버에 연결
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "[C++] 서버 연결 실패 (Python 스크립트가 실행 중인가요?)" << std::endl;
        close(sock);
        sock = -1;
        return;
    }
    
    std::cout << "[C++] Python 카메라 서버 연결 완료" << std::endl;
}

nlohmann::json CamController::getArucoData()
{
    if (sock < 0) {
        std::cerr << "[C++] 소켓이 연결되지 않음" << std::endl;
        return json::object();
    }
    
    std::string msg = "get_marker_data";
    ssize_t sent = send(sock, msg.c_str(), msg.length(), 0);
    
    if (sent < 0) {
        std::cerr << "[C++] 데이터 전송 실패" << std::endl;
        return json::object();
    }

    // JSON 데이터 수신
    // Python이 분석하는 동안(1초) 여기서 블로킹(대기) 됩니다.
    memset(buffer, 0, sizeof(buffer));
    int valread = read(sock, buffer, sizeof(buffer) - 1);
    
    if (valread > 0) {
        try {
            // 문자열을 JSON 객체로 파싱
            std::string json_str(buffer, valread);
            json j = json::parse(json_str);
            return j;
        } catch (json::parse_error& e) {
            std::cerr << "[C++] JSON 파싱 에러: " << e.what() << std::endl;
            return json::object();
        }
    } else if (valread == 0) {
        std::cerr << "[C++] 연결이 닫힘" << std::endl;
    } else {
        std::cerr << "[C++] 데이터 수신 실패" << std::endl;
    }

    return json::object();
}

std::vector<std::pair<int,int>> CamController::updateInventory()
{
    std::vector<std::pair<int,int>> inventoryDatas;
    // aruco 번호, 상자 갯수  반환
    nlohmann::json arucoData = getArucoData();

    for (auto& element : arucoData.items()) {
            std::string id_str = element.key();
            int count = element.value();
            
            std::cout << "  -> 마커 ID: " << id_str << ", 개수: " << count << std::endl;
            int id = std::stoi(id_str);
            inventoryDatas.push_back({id, count});
            
        }
    return inventoryDatas;
}
CamController::~CamController()
{
    std::cout << "카메라 컨트롤러 소멸" << std::endl;
    if (sock >= 0) {
        close(sock);
    }
}