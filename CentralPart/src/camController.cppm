module;

#include <iostream>
#include <vector>
#include <utility>
#include <sys/socket.h>
#include <sys/un.h>
#include <nlohmann/json.hpp> 

export module camController;

export class CamController{
    private:
        int sock = 0;
        struct sockaddr_un serv_addr;
        const char* socket_path = "/tmp/aruco_socket";
        char buffer[4096] = {0};
        nlohmann::json getArucoData();        
    public:
        CamController();

        bool CameraShot();
        std::vector<std::pair<int,int>> updateInventory();
        ~CamController();
};