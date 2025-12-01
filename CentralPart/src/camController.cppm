module;

#include<iostream>

export module camController;

export class CamController{
    private:
        int camera_id;
    public:
        CamController(int cam_id);
        bool CameraShot();
        ~CamController();
};