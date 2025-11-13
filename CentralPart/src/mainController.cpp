module;

#include <memory>
#include <string>
#include <iostream>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <chrono>

module mainController;

import webController;
import mapper;
import moveController;

//생성자
MainController::MainController() : webCtrl(nullptr), mapper(nullptr), camCtrl(nullptr), running(false)
{   
    // Mapper 초기화
    mapper = std::make_unique<Mapper>();
    
    // CamController 초기화
    camCtrl = std::make_unique<CamController>(0);
}

//소멸자
MainController::~MainController()
{
    stopWebServer();
}

bool MainController::initWebController(const std::string& socket_path)
{
    webCtrl = std::make_unique<WebController>(socket_path.c_str());
    return webCtrl->is_ready();
}


bool MainController::initMoveController(int serial_fd, int baud_rate)
{
    moveCtrl = std::make_unique<MoveController>(serial_fd, baud_rate);
    return moveCtrl->isRead();
}

// 서버 스레드 시작
void MainController::startWebServerThread()
{
    if (running) {
        std::cerr << "서버가 이미 실행 중입니다." << std::endl;
        return;
    }
    
    running = true;
    serverThread = std::thread(&MainController::serverThreadFunction, this);
    std::cout << "웹 서버 스레드 시작됨" << std::endl;
}

// 서버 스레드 종료
void MainController::stopWebServer()
{
    if (!running) return;
    
    running = false;
    
    if (serverThread.joinable()) {
        serverThread.join();
    }
    
    std::cout << "웹 서버 스레드 종료됨" << std::endl;
}

// 서버 스레드 함수 (백그라운드에서 실행)
void MainController::serverThreadFunction()
{
    while(running)
    {
        if(!webCtrl->accept_connection())
        {
            std::cerr << "클라이언트 연결 실패, 재시도..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        char buffer[1024];
        if(webCtrl->receive_message(buffer, sizeof(buffer)))
        {
            // 받은 메시지를 우선순위 큐에 추가
            std::string receivedData(buffer);
            pushMessage(1, receivedData);  // 우선순위 1로 설정
            

            //TODO : message를 처리할 수 있는지 파악해야 함.
            //가령 실제 움직일 수 없다고 moveController가 파악한 경우 해결 방법
            if(receivedData == "up" || receivedData == "down" || receivedData == "left" 
                || receivedData == "right"){
                if(mapper->IsMappingDone()){
                    webCtrl->send_response("MAPPINGDONE");
                    continue;
                }
                else{
                    // movecontroller 넣을 곳
                    webCtrl->send_response("ACK");
                    continue;
                }   
            }
            else if (receivedData == "featureShot"){
                // 임시로 항상 성공 응답 보내기 (카메라 기능이 완전히 구현될 때까지)
                std::cout << "카메라 촬영 요청 받음 (임시 성공 응답)" << std::endl;
                webCtrl->send_response("FEATURESHOT_OK");
                continue;
            }
            else if(receivedData == "remapping")
            {
                webCtrl->send_response("REMAPPING_QUEUED");
                continue;
            }
            else if(receivedData == "requestMap"){
                // 맵 데이터를 JSON 형식으로 전송
                std::string mapJson = getMapAsJson();
                webCtrl->send_response(mapJson.c_str());
                continue;
            }
        }
        
        // 각 요청 처리 후 잠시 대기 (다음 연결을 위해)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// 메시지 큐에 추가
void MainController::pushMessage(int priority, const std::string& data)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    messageQueue.push({priority, data});
    queueCV.notify_one();  // 대기 중인 스레드 깨우기
}

// 메시지 큐에서 가져오기 (블로킹)
bool MainController::popMessage(Message& msg, int timeout_ms)
{
    std::unique_lock<std::mutex> lock(queueMutex);
    
    if (timeout_ms < 0) {
        // 무한 대기
        queueCV.wait(lock, [this]{ return !messageQueue.empty(); });
    } else {
        // 타임아웃 대기
        if (!queueCV.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                              [this]{ return !messageQueue.empty(); })) {
            return false;  // 타임아웃
        }
    }
    
    if (!messageQueue.empty()) {
        msg = messageQueue.top();
        messageQueue.pop();
        return true;
    }
    
    return false;
}

// 메시지 큐 크기
size_t MainController::getQueueSize()
{
    std::lock_guard<std::mutex> lock(queueMutex);
    return messageQueue.size();
}

std::string MainController::interpretMessage()
{
    Message msg;
    if(popMessage(msg, 5000)){
        std::string ACKMSG = "";
        if(msg.data == "up" || msg.data == "down" || msg.data == "left" || msg.data == "right" || msg.data == "doneMapping"){
            // moveController에게 실제로 움직일 수 있는지 확인 받고 오기.
            bool moveSuccess = true;
            //bool moveSuccess = moveCtrl->processCommand(msg.data);
            if(moveSuccess) 
            {
                //아래에 있는 ACKMSG 파라미터는 done인지 아닌지 확인하고 오기 위함.
                ACKMSG = mapper->getMappingMessages(msg.data.c_str());
                if(ACKMSG == "DONEMAPPING"){
                    currentMode = Mode::NAVIGATING;

                    return ACKMSG;
                }
                else {
                    // MOVE OK,
                    currentMode = Mode::MAPPING;
                    return ACKMSG;
                }
            }
            else{
                // MOVE FAIL, 추가
                std::cout << "MoveController: 장애물 있음 [" << msg.data << "]" << std::endl;
                std::cout << "DELETE ME";
                return "MOVEFAIL";
            }
        }
        else if(msg.data == "featureShot"){
            // CameraController 통해 사진 받아서 저장하는 로직 처리하기.
            ACKMSG = mapper->getMappingMessages(msg.data.c_str());
            return ACKMSG;
        }
        else if(msg.data == "remapping"){
            
            mapper = std::make_unique<Mapper>();
            
            currentMode = Mode::MAPPING;
            
            return "REMAPPING_STARTED";
        }
        else if(msg.data == "requestMap"){
            // 맵 데이터 요청 처리
            return getMapAsJson();
        }
        else{
            std::cout << "Unknown command: " << msg.data << std::endl;
            ACKMSG = "UNKNOWNCOMMAND";
            return ACKMSG;
        }        
    }
    else{
        return "NOMESSAGE";
    }
}

std::string MainController::getMapAsJson()
{
    auto mapData = mapper->getMap();
    if (mapData.empty()) {
        return "NO_MAP";
    }
    
    std::string json = "[";
    for (size_t i = 0; i < mapData.size(); ++i) {
        if (i > 0) json += ",";
        json += "[";
        for (size_t j = 0; j < mapData[i].size(); ++j) {
            if (j > 0) json += ",";
            json += std::to_string(mapData[i][j]);
        }
        json += "]";
    }
    json += "]";
    
    return json;
}