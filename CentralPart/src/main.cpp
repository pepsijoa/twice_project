import webController;
import mainController;
#include <iostream>
#include <thread>
#include <chrono>

int main()
{
    MainController mainCtrl;
    std::cout << "hello" << std::endl;
    // 웹 컨트롤러 초기화
    if(!mainCtrl.initWebController("/tmp/flaskToCPP.sock")){
        std::cerr << "웹 컨트롤러 초기화 실패" << std::endl;
        return -1;
    }
    
    // 웹 서버를 별도 스레드에서 시작
    mainCtrl.startWebServerThread();

    while(true) {
        mainCtrl.interpretMessage();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    mainCtrl.stopWebServer();
    
    return 0;
}