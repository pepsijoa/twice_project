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

//생성자
MainController::MainController() : webCtrl(nullptr), mapper(nullptr), running(false)
{   
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
            
            webCtrl->send_response("ACK");
            std::cout << "메시지 수신 및 큐에 추가: " << receivedData << std::endl;
        }
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

void MainController::interpretMessage()
{
    Message msg;
    if(popMessage(msg, 5000)){
        std::cout << " 우선순위 :" << msg.priority << std::endl;
        std::cout << " 데이터 : " << msg.data << std::endl;
        std::cout << " 남은 메시지 : " << getQueueSize() << std::endl;

        if(msg.data == "up" || msg.data == "down" || msg.data == "left" || msg.data == "right" || msg.data == "doneMapping"){
            mapper->getMappingMessages(msg.data.c_str());
        }
        else{
            std::cout << "Unknown command: " << msg.data << std::endl;
        }
    }
    else{
        std::cout << "." << std::flush;
    }
}