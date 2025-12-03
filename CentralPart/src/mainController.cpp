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
                    //bool moveSuccess = true;
                    std::cout << "After " << receivedData << std::endl;
                    //bool moveSuccess = true; //임시로 항상 이동 성공이라고 가정
                    lastOrientation = receivedData;
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

            else if (receivedData == "doneMapping") {
                currentMode = Mode::SEARCHING;
                pushMessage(1, receivedData);
                webCtrl->send_response("DONEMAPPING_QUEUED");
            }

            else if (receivedData.rfind("featureShot/", 0) == 0) { 
                std::string featureName = receivedData.substr(std::string("featureShot/").length());
                std::cout << "특징점 촬영 요청, 이름: " << featureName << std::endl;

                //bool checkstored = moveCtrl->processCommand("mapping_feature_arrive", lastOrientation);
                
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
                currentMode = Mode::MAPPING;
                webCtrl->send_response("REMAPPING_QUEUED");
                continue;
            }
            else if(receivedData == "requestMap"){
                // 맵 데이터를 JSON 형식으로 전송
                std::string mapJson = getMapAsJson();
                webCtrl->send_response(mapJson.c_str());
                continue;
            }
            else if(receivedData.rfind("MoveTo/", 0) == 0){
                if(currentMode == Mode::SEARCHING){
                    currentMode = Mode::NAVIGATING;
                    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                }
                std::string featureName = receivedData.substr(std::string("MoveTo/").length());
                
                //mapper로 특정 지점까지의 경로 탐색 요청
                std::vector<std::pair<int,int>> path = mapper->findNavigatingPathBFS(featureName);
                
                
                if(path.empty()) {
                    webCtrl->send_response("MoveToFAIL:경로를 찾을 수 없습니다");
                    continue;
                }
                
                // 전체 경로 개수를 먼저 전송
                std::string pathInfo = "MoveToSTART:" + std::to_string(path.size() - 1);
                webCtrl->send_response(pathInfo.c_str());
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                
                //목적지까지의 경로를 배열에 얻은 다음에 moveController로 전송
                std::pair<int,int> pre = path[0];
                std::string direction = "";
                int stepCount = 0;
                
                for(size_t i = 1; i < path.size(); i++){
                    direction = mapper->getDirection(pre, path[i]);
                    pre = path[i];
                    stepCount++;
                    
                    
                    // moveSuccess = moveCtrl->processCommand(direction);
                    bool moveSuccess = true; //임시로 항상 이동 성공이라고 가정
                    lastOrientation = direction;
                    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

                    if(moveSuccess == false)
                    {
                        std::string failMsg = "MoveToFAIL:" + direction + ":" + std::to_string(stepCount);
                        webCtrl->send_response(failMsg.c_str());
                        break;
                    }
                    else if (moveSuccess == true)
                    {
                        std::string okMsg = "MoveToOK:" + direction + ":" + std::to_string(stepCount);
                        webCtrl->send_response(okMsg.c_str());
                        mapper->updateSearchingResult(pre, 1);
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                }
                // moveCtrl한테 도착 했으니 각도 알려주고 camera 돌리기
                bool orientSuccess = moveCtrl->processCommand("mapped_feature_arrive", lastOrientation, mapper->getIndexOfFeatureByName(featureName));
                //
                webCtrl->send_response("MoveToDONE");
                currentMode = Mode::NAVIGATINGDONE;
                continue;
            }
            else if(receivedData == "navigatedone"){
                // NAVIGATINGDONE 모드에서 SEARCHING 모드로 전환
                if(currentMode == Mode::NAVIGATINGDONE){
                    currentMode = Mode::SEARCHING;
                    webCtrl->send_response("OK");
                    std::cout << "✅ Searching 모드로 전환됨" << std::endl;
                } else {
                    webCtrl->send_response("FAIL:현재 상태에서 전환할 수 없습니다");
                }
                continue;
            }


        }
        
        // 각 요청 처리 후 즉시 다음 연결 대기 (대기 시간 최소화)
        // std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void MainController::startSearchingPath() {

    auto featureLocations = mapper->getFeatureInfo();
    
    // 스레드가 이미 실행 중이면 중복 생성 방지
    if(searchingThread.joinable()) {
        return;
    }
    
    
    searchingThread = std::thread([this]() {
        std::string targetFeature = "";  // 현재 목표로 하는 feature
        
        while(true) 
        {
            // 모드가 SEARCHING이 아니면 일시정지
            if(currentMode != Mode::SEARCHING) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
            
            auto currentFeatures = mapper->getFeatureInfo();
            const int totalFeatures = currentFeatures.size();
            
            // 방문한 특징점 목록 가져오기
            std::vector<std::string> currentVisited;
            {
                std::lock_guard<std::mutex> lock(visitedMutex);
                currentVisited = visitedFeatures;
            }
            
            
            // 모든 특징점을 방문했으면 초기화하고 다시 시작
            if(currentVisited.size() >= static_cast<size_t>(totalFeatures)) {
                {
                    std::lock_guard<std::mutex> lock(visitedMutex);
                    visitedFeatures.clear();
                    currentVisited.clear();
                }
                targetFeature = "";  // 목표 초기화
                continue;  // 즉시 다음 루프로 (다시 방문 상태 체크)
            }
            
            // 현재 위치 확인
            auto currentPos = mapper->getCurrentLocation();
            
            // 현재 위치가 특징점인지 확인하고 방문 처리
            bool currentIsFeature = false;
            for(const auto& feature : currentFeatures) {
                if(feature.position == currentPos) {
                    currentIsFeature = true;
                    bool alreadyVisited = false;
                    for(const auto& visited : currentVisited) {
                        if(feature.name == visited) {
                            alreadyVisited = true;
                            break;
                        }
                    }
                    
                    if(!alreadyVisited) {
                        {
                            std::lock_guard<std::mutex> lock(visitedMutex);
                            visitedFeatures.push_back(feature.name);
                        }
                        // 방문 후 즉시 다음 목표로 이동하기 위해 continue
                        targetFeature = "";
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        continue;
                    }
                    
                    // 이미 방문한 특징점이면서 목표였던 경우
                    if(targetFeature == feature.name) {
                        targetFeature = "";
                        continue;
                    }
                    break;
                }
            }
            
            // 다음 목표 특징점으로 경로 찾기 (현재 방문 상태 업데이트 후)
            {
                std::lock_guard<std::mutex> lock(visitedMutex);
                currentVisited = visitedFeatures;
            }
            
            std::vector<std::pair<int,int>> path = mapper->moveToNearestFeaturePoint(currentVisited);
            
            if(path.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
            
            // 목표 특징점 이름 찾기
            std::pair<int,int> destination = path.back();
            for(const auto& feature : currentFeatures) {
                if(feature.position == destination) {
                    targetFeature = feature.name;
                    break;
                }
            }
            
            // 경로를 따라 이동 (모드 변경 시 중단 가능)
            std::pair<int,int> pre = path[0];
            bool pathCompleted = true;
            
            for (size_t i = 1; i < path.size(); i++) {
                // 이동 중 모드가 변경되면 중단
                if(currentMode != Mode::SEARCHING) {
                    pathCompleted = false;
                    break;
                }
                
                std::pair<int,int> point = path[i];
                std::string direction = mapper->getDirection(pre, point);
                
                
                // 실제 moveController 호출 (주석 해제 필요)
                //bool moveSuccess = moveCtrl->processCommand(direction);
                bool moveSuccess = true; //임시로 항상 이동 성공이라고 가정
                lastOrientation = direction;

                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                
                if(moveSuccess == false) {
                    mapper->updateSearchingResult(point, -1);
                    pathCompleted = false;
                    targetFeature = "";  // 실패 시 목표 초기화
                    break;
                }
                else {
                    mapper->updateSearchingResult(point, 1);
                  }
                
                pre = point;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
        }
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
                startSearchingPath();
                return ACKMSG;
            }

            else {
                return ACKMSG;
            }
            
            return ACKMSG;  // 기본 반환값 추가

        }
        
        else if(msg.data.rfind("featureShot/", 0) == 0){
            ACKMSG = mapper->getMappingMessages(msg.data.c_str());
            return ACKMSG;
        }
        else if(msg.data == "remapping"){
            mapper = std::make_unique<Mapper>();            
            return "REMAPPING_STARTED";
        }
        else if(msg.data == "requestMap"){
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

    // 맵 요청 시 현재 상태로 맵 업데이트 (현재 위치 포함)
    mapper->updateMapWithCurrentState();
    
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