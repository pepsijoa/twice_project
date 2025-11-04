module;

#include <cstring>
#include <iostream>

module mapper;

Mapper::Mapper() {
    
}

Mapper::~Mapper() {
}

void Mapper::getMappingMessages(const char* msg)
{
    if(!doneMapping)
    {
        if(strcmp(msg, "left") == 0){
            currentLocation.first -= 1;
            if(currentLocation.first < mostLeft){
                mostLeft = currentLocation.first;
            }
            locations.push_back(currentLocation);
        } else if(strcmp(msg, "right") == 0){
            currentLocation.first += 1;
            locations.push_back(currentLocation);
        } else if(strcmp(msg, "up") == 0){
            currentLocation.second += 1;
            locations.push_back(currentLocation);
        } else if(strcmp(msg, "down") == 0){
            currentLocation.second -= 1;
            if(currentLocation.second < mostDown){
                mostDown = currentLocation.second;
            }
            locations.push_back(currentLocation);
        }
        else if(strcmp(msg, "doneMapping") == 0){
            doneMapping = true;
            for(auto& loc : locations){
                loc.first = loc.first + (mostLeft * -1);
                loc.second = loc.second + (mostDown * -1);
            }
            return;
        }
        else{
            std::cout << "Invalid mapping command: " << msg << std::endl;
            return;
        }
    }

    else{
        std::cout << "Mapping is already done." << std::endl;
        return;
    }
}