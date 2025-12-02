module;

#include<iostream>
#include<vector>

export module mapper;

export class Mapper{
    private:
        int mostLeft = 0, mostDown = 0;
        int mostRight = 0, mostUp = 0;
        bool doneMapping = false;
        std::vector<std::vector<int>> map;

        std::vector<std::pair<int, int>> locations{{0,0}};
        
        struct FeaturePoint{
            std::pair<int, int> position;
            std::string name;
        };

        std::vector<FeaturePoint> featureLocations;
        std::vector<FeaturePoint> sendFeatureinfo;

        // std::vector<std::pair<int, int>> featureLocations;
        std::pair<int, int> currentLocation{0,0};
        bool AmIFeaturePoint();
    public:
        Mapper();
        void updateMapWithCurrentState();  // 실시간 맵 업데이트 함수
        std::string getMappingMessages(const char* msg);
        bool IsMappingDone();
        std::vector<std::pair<int,int>> moveToNearestFeaturePoint(const std::vector<std::string>& visitedFeatures);
        void updateSearchingResult(std::pair<int,int> locate, int state);
        std::vector<std::vector<int>> getMap() const {return map;}
        std::vector<FeaturePoint> getFeatureInfo() const { return sendFeatureinfo; }
        std::pair<int, int> getCurrentLocation() const { return currentLocation; }
        std::pair<int, int> getPositionByName(const std::string& name) const;
        std::vector<std::pair<int, int>> findSearchingPathBFS(std::pair<int,int> end);
        std::vector<std::pair<int, int>> findNavigatingPathBFS(std::string featureName);
        std::string getDirection(std::pair<int,int> start, std::pair<int,int> end);
        ~Mapper();
};