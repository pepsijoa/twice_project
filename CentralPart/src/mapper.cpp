module;

#include <cstring>
#include <iostream>
#include <vector>
#include <queue>

module mapper;

Mapper::Mapper() {
    
}

Mapper::~Mapper() {
}
/*
msg로 doneMapping -> MAPPINGOK
이후에 왼, 오, 위, 아래 들어오면, MAPPINGDONE
*/
std::string Mapper::getMappingMessages(const char* msg)
{
    if(!doneMapping)
    {
        if(strcmp(msg, "left") == 0){
            currentLocation.second -= 1;
            if(currentLocation.second < mostLeft){
                mostLeft = currentLocation.second;
            }
            locations.push_back(currentLocation);
        } else if(strcmp(msg, "right") == 0){
            currentLocation.second += 1;
            if(currentLocation.second > mostRight){
                mostRight = currentLocation.second;
            }
            locations.push_back(currentLocation);
        } else if(strcmp(msg, "up") == 0){
            currentLocation.first += 1;
            if(currentLocation.first > mostUp){
                mostUp = currentLocation.first;
            }
            locations.push_back(currentLocation);
        } else if(strcmp(msg, "down") == 0){
            currentLocation.first -= 1;
            if(currentLocation.first < mostDown){
                mostDown = currentLocation.first;
            }
            locations.push_back(currentLocation);
        }
        else if(strcmp(msg, "doneMapping") == 0){
            doneMapping = true;
            
            for(auto& loc : locations){
                loc.second = loc.second + (mostLeft * -1);
                loc.first = loc.first + (mostDown * -1);
            }
            
            //feature 지점 또한 보정
            for(auto& loc : featureLocations){
                loc.second = loc.second + (mostLeft * -1);
                loc.first = loc.first + (mostDown * -1);
            }
            
            //현재 위치 보정.
            currentLocation.first = currentLocation.first + (mostDown * -1);
            currentLocation.second = currentLocation.second + (mostLeft * -1);

            createMap();
            return "MAPPINGOK";
        }
        
        else if(strcmp(msg, "featureShot") == 0){
            featureLocations.push_back(currentLocation);
            std::cout << "Feature shot command received." << std::endl;
            return "FEATURESHOTOK";
        }

        else{
            std::cout << "Invalid mapping command: " << msg << std::endl;
            return "INVALID";
        }

        return "OK";
    }

    else{
        std::cout << "Mapping is already done." << std::endl;
        return "MAPPINGDONE";
    }
}

void Mapper::createMap()
{
    if(locations.empty()) {
        std::cout << "No locations to create map" << std::endl;
        return;
    }
    
    // locations에서 최대 x, y 좌표 찾기
    int maxX = 0, maxY = 0;
    for(const auto& loc : locations) {
        if(loc.second > maxX) maxX = loc.second;
        if(loc.first > maxY) maxY = loc.first;
    }

    // 맵 크기 설정 (0부터 시작하므로 +1)
    int mapWidth = maxX + 1;
    int mapHeight = maxY + 1;
    
    map.clear();
    map.resize(mapHeight, std::vector<int>(mapWidth, 0));
    map[0][0] = 1; 
    // locations에 있는 좌표들에 1 할당
    for(const auto& loc : locations) {
        int x = loc.second;
        int y = loc.first;
        if(x >= 0 && x < mapWidth && y >= 0 && y < mapHeight) {
            map[y][x] = 1;
        }
    }

    // featureLocations에 있는 좌표들에 2 할당
    for(const auto& loc : featureLocations) {
        int x = loc.second;
        int y = loc.first;
        if(x >= 0 && x < mapWidth && y >= 0 && y < mapHeight) {
            map[y][x] = 2;
        }
    }
}

void Mapper::showMap()
{
    std::cout << "Map display complete." << std::endl;
    for(int y = map.size() - 1; y >= 0; --y) {
        for(int x = 0; x < map[y].size(); ++x) {
            std::cout << map[y][x] << " ";
        }
        std::cout << std::endl;
    }
    
}
bool Mapper::IsMappingDone()
{
    return doneMapping;
}