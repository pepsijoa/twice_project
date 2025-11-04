import webController;
import mainController;
#include <iostream>
#include <thread>
#include <chrono>

int main()
{
    MainController mainCtrl;
    
    // 웹 컨트롤러 초기화
    if(!mainCtrl.initWebController("/tmp/flaskToCPP.sock")){
        std::cerr << "웹 컨트롤러 초기화 실패" << std::endl;
        return -1;
    }
    
    // 웹 서버를 별도 스레드에서 시작
    mainCtrl.startWebServerThread();
    
    while(true) {
        Message msg;
        
        // 메시지 큐에서 메시지 가져오기 (5초 타임아웃)
        if (mainCtrl.popMessage(msg, 5000)) {
            std::cout << "\n[메시지 처리]" << std::endl;
            std::cout << "  우선순위: " << msg.priority << std::endl;
            std::cout << "  데이터: " << msg.data << std::endl;
            std::cout << "  남은 메시지: " << mainCtrl.getQueueSize() << std::endl;
            
            // 여기서 메시지에 따른 로직 처리
            // 예: msg.data에 따라 다른 작업 수행
            
        } else {
            // 타임아웃 - 주기적으로 상태 확인 가능
            std::cout << "." << std::flush;
        }
        
        // Ctrl+C로 종료할 수 있도록 (또는 특정 종료 조건 추가)
    }
    
    // 정리 (실제로는 시그널 핸들러 등으로 종료 처리)
    mainCtrl.stopWebServer();
    
    return 0;
}