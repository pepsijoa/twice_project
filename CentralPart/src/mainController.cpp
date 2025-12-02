module;

#include <memory>
#include <string>
#include <iostream>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <sstream>

module mainController;

import webController;
import mapper;
import moveController;
import camController;

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


bool MainController::initMoveController(const std::string& port_name, int baud_rate)
{
    moveCtrl = std::make_unique<MoveController>();
    
    if (!moveCtrl->openPort(port_name, baud_rate)) {
        std::cerr << "MainController: MoveController 포트 열기 실패" << std::endl;
        return false;
    }
    if (moveCtrl->openPort(port_name, baud_rate)) {
        std::cerr << "포트 열림. 아두이노 부팅 대기 중" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2)); // 2초 대기
    }
    
    return moveCtrl->isReady();
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
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        char buffer[1024];
        if(webCtrl->receive_message(buffer, sizeof(buffer)))
        {
            
            std::string receivedData(buffer);
            
            //TODO : message를 처리할 수 있는지 파악해야 함.
            //가령 실제 움직일 수 없다고 moveController가 파악한 경우 해결 방법
            if(receivedData == "up" || receivedData == "down" || receivedData == "left" 
                || receivedData == "right"){
                if(mapper->IsMappingDone()){    
                    webCtrl->send_response("MAPPINGDONE");
                    continue;   
                }
                else{
                    
                    
                    std::cout << "Before " << receivedData << std::endl;

                    bool moveSuccess = moveCtrl->processCommand(receivedData);
                    
                    std::cout << "After " << receivedData << std::endl;
                    //bool moveSuccess = true; //임시로 항상 이동 성공이라고 가정
                    if(moveSuccess){
                        webCtrl->send_response("ACK");
                        pushMessage(1, receivedData);
                    }
                    else{
                        webCtrl->send_response("MOVEFAIL");
                    }

                    continue;
                }   
            }
            else if (receivedData.rfind("featureShot/", 0) == 0) { 
                std::string featureName = receivedData.substr(std::string("featureShot/").length());
                std::cout << "특징점 촬영 요청, 이름: " << featureName << std::endl;

                //bool checkstored = moveCtrl->processCommand("mapping_feature");
                bool camSuccess = false;
                bool checkstored = true;
                
            
                if(checkstored == true)
                {
                    camSuccess = camCtrl->CameraShot();    
                    if(camSuccess)
                    {
                        pushMessage(2, receivedData); // 필요시 featureName만 push 가능
                    }
                    else{
                        webCtrl->send_response("FEATURESHOT_FAIL");
                        continue;
                    }
                }
                else{
                    webCtrl->send_response("FEATURESHOT_FAIL");
                    continue;
                }
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
        
        // 각 요청 처리 후 즉시 다음 연결 대기 (대기 시간 최소화)
        // std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void MainController::startNavigatingPath() {

    auto featureLocations = mapper->getFeatureInfo();

    //const std::vector<std::pair<int,int>>& path
    navigatingActive = true;
    navigatingThread = std::thread([this, featureLocations]() {
        for (const auto& loc :  featureLocations)
        {
            std::pair<int,int> goal = loc.position;
            std::vector<std::pair<int, int>> path = mapper->findNavigatingPathBFS(goal);
            for (const auto& point : path) {
                if (currentMode != Mode::NAVIGATING || !navigatingActive) break;
                // moveController에 명령 전송
                bool moveSuccess = false;
                
                //moveSuccess = moveCtrl->moveTo(point);
                if(moveSuccess == false)
                {
                    mapper->updateNavigateResult(point, -1);
                }
                else if (moveSuccess == true)
                {
                    mapper->updateNavigateResult(point, 1);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            }
        }
        navigatingActive = false;
    });
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
            
            ACKMSG = mapper->getMappingMessages(msg.data.c_str());
            if(ACKMSG == "DONEMAPPING"){
                currentMode = Mode::NAVIGATING;
                startNavigatingPath();
                return ACKMSG;
            }
            else {
                // MOVE OK,
                currentMode = Mode::MAPPING;
                return ACKMSG;
            }
            

            // std::cout << "MoveController: 장애물 있음 [" << msg.data << "]" << std::endl;
            // std::cout << "DELETE ME";
            // return "MOVEFAIL";

        }
        
        else if(msg.data.rfind("featureShot/", 0) == 0){
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
    // 데이터 읽는 동안 맵이 변경되지 않도록 Mutex 잠금 권장
    // std::lock_guard<std::mutex> lock(mapperMutex); 

    auto mapData = mapper->getMap();
    auto featureData = mapper->getFeatureInfo();

    if (mapData.empty()) {
        return "NO_MAP";
    }

    std::stringstream ss;
    ss << "{ \"grid\": [";

    for (size_t i = 0; i < mapData.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "[";
        for (size_t j = 0; j < mapData[i].size(); ++j) {
            if (j > 0) ss << ",";
            ss << std::to_string(mapData[i][j]);
        }
        ss << "]";
    }
    ss << "], \"features\": [";

    for (size_t i = 0; i < featureData.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "{\"y\": " << featureData[i].position.first 
           << ",\"x\": " << featureData[i].position.second 
           << ",\"name\": \"" << featureData[i].name << "\"}";
    }
    
    ss << "] }";

    return ss.str();
}