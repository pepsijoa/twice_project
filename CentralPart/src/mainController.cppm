
module;

#include <memory>
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>

export module mainController;

import webController;
import mapper;
import camController;
import moveController;

// 메시지 구조체
export struct Message {
    int priority;           // 우선순위 (낮을수록 높은 우선순위)
    std::string data;       // 메시지 데이터
    
    // 우선순위 큐를 위한 비교 연산자 (priority가 낮을수록 우선)
    bool operator<(const Message& other) const {
        return priority > other.priority;  // 역순 정렬
    }
};

export class MainController {
private:
    enum Mode{
        MAPPING = 0,
        SEARCHING = 1,
        NAVIGATING = 2
    };

    Mode currentMode = MAPPING;

    std::unique_ptr<WebController> webCtrl;
    std::unique_ptr<Mapper> mapper;
    std::unique_ptr<CamController> camCtrl;
    std::unique_ptr<MoveController> moveCtrl;


    std::thread serverThread;
    bool running;
    
    // 공유 우선순위 큐
    std::priority_queue<Message> messageQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    
    // 서버 스레드 함수
    void serverThreadFunction();

    // 메시지 큐 크기
    size_t getQueueSize();

    // 메시지 큐에서 가져오기 (블로킹)
    bool popMessage(Message& msg, int timeout_ms = -1);
    
public:
    MainController();
    ~MainController();
    
    bool initWebController(const std::string& socket_path);
    bool initMoveController(int serial_fd, int baud_rate);

    // 서버 스레드 시작
    void startWebServerThread();
    
    // 서버 스레드 종료
    void stopWebServer();
    
    // 메시지 큐에 추가
    void pushMessage(int priority, const std::string& data);

    // 메시지 큐에서 가져온 데이터 해석
    std::string interpretMessage();

    
    // 맵 데이터를 JSON 형식으로 반환
    std::string getMapAsJson();
};