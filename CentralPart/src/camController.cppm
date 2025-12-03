module;

#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <sys/socket.h>
#include <sys/un.h>

export module camController;

export class CamController{
    private:
        int sock = -1;
        struct sockaddr_un serv_addr;
        const char* socket_path = "/tmp/aruco_socket";
        std::string getArucoDataRaw();
        bool connectToServer();
    public:
        CamController();
        bool init(int max_retries = 5, int retry_delay_ms = 500);
        bool isConnected() const;
        bool CameraShot();
        std::vector<std::pair<int,int>> updateInventory();
        ~CamController();
};