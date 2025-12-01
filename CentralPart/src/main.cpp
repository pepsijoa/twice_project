#include <iostream>
#include <thread>
#include <chrono>
#include <string>

import webController;
import mainController;
import moveController;

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

    const std::string ARDUINO_PORT = "/dev/ttyACM0";
    const int ARDUINO_BAUD = 115200;
    while(!mainCtrl.initMoveController(ARDUINO_PORT, ARDUINO_BAUD))
    {
        std::cerr << "Main: MoveController 초기화 재시도..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // get msg from web controller
    // send msg to arudino controller (move controller)

    while(true) {
        std::string interpretAck = mainCtrl.interpretMessage();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    }
    
    mainCtrl.stopWebServer();
    
    return 0;
}
