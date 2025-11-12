module;

#include <iostream>


module camController;

CamController::CamController(int cam_id) : camera_id(cam_id)
{
    std::cout << "카메라 컨트롤러 생성, 카메라 ID: " << camera_id << std::endl;
}

int CamController::CameraShot()
{
    std::cout << "카메라 촬영 수행, 카메라 ID: " << camera_id << std::endl;
    // 실제 카메라 촬영 로직 구현 필요
    return 0; // 촬영 성공 시 0 반환
}
CamController::~CamController()
{
    std::cout << "카메라 컨트롤러 소멸, 카메라 ID: " << camera_id << std::endl;
}