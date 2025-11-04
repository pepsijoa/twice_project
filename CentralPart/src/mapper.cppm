module;

#include<iostream>
#include<vector>

export module mapper;

export class Mapper{
    private:
        int mostLeft = 0, mostDown = 0;

        bool doneMapping = false;
        std::vector<std::pair<int, int>> locations;
        std::pair<int, int> currentLocation{0,0};

    public:
        Mapper();
        void getMappingMessages(const char* msg);
        ~Mapper();
};