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
        std::vector<std::pair<int, int>> locations;
        std::vector<std::pair<int, int>> featureLocations;
        std::pair<int, int> currentLocation{0,0};
        void createMap();
        
    public:
        Mapper();
        std::string getMappingMessages(const char* msg);
        bool IsMappingDone();
        void showMap();
        std::vector<std::pair<int, int>> findSearchingPathBFS(const std::vector<std::vector<int>>& map, std::pair<int,int> start, std::pair<int,int> end);
        std::vector<std::pair<int, int>> findNavigatingPathBFS(const std::vector<std::vector<int>>& map, std::pair<int,int> start, std::pair<int,int> end);
        ~Mapper();
};