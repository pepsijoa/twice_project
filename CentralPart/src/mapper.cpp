module;

#include <cstring>
#include <iostream>
#include <vector>
#include <queue>
#include<algorithm>
#include <string>

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
    std::string msg_s(msg);

    if(!doneMapping)
    {
        if(strcmp(msg, "left") == 0){
            currentLocation.second -= 1;
            if(currentLocation.second < mostLeft){
                mostLeft = currentLocation.second;
            }
            locations.push_back(currentLocation);
            
            // 실시간 맵 업데이트
            updateMapWithCurrentState();
            
            return "OK";
        } 
        
        else if(strcmp(msg, "right") == 0){
            currentLocation.second += 1;
            if(currentLocation.second > mostRight){
                mostRight = currentLocation.second;
            }
            locations.push_back(currentLocation);
            
            // 실시간 맵 업데이트
            updateMapWithCurrentState();
            
            return "OK";
        } 
        
        else if(strcmp(msg, "up") == 0){
            currentLocation.first += 1;
            if(currentLocation.first > mostUp){
                mostUp = currentLocation.first;
            }
            locations.push_back(currentLocation);
            
            // 실시간 맵 업데이트
            updateMapWithCurrentState();
            
            return "OK";
        } 
        
        else if(strcmp(msg, "down") == 0){
            currentLocation.first -= 1;
            if(currentLocation.first < mostDown){
                mostDown = currentLocation.first;
            }
            locations.push_back(currentLocation);
            
            // 실시간 맵 업데이트
            updateMapWithCurrentState();
            
            return "OK";
        }

        else if(strcmp(msg, "doneMapping") == 0){
            doneMapping = true;
            
            // 실제 좌표 데이터 보정 (영구적으로 변경)
            for(auto& loc : locations){
                loc.second = loc.second + (mostLeft * -1);
                loc.first = loc.first + (mostDown * -1);
            }
            
            // feature 지점 좌표 보정
            for(auto& loc : featureLocations){
                loc.position.second = loc.position.second + (mostLeft * -1);
                loc.position.first = loc.position.first + (mostDown * -1);
            }
            
            // 현재 위치 보정
            currentLocation.first = currentLocation.first + (mostDown * -1);
            currentLocation.second = currentLocation.second + (mostLeft * -1);

            // 경계값 리셋 (이제 모든 좌표가 0 이상)
            mostLeft = 0;
            mostDown = 0;
            mostRight = currentLocation.second;
            mostUp = currentLocation.first;
            
            // 최종 맵 생성 (updateMapWithCurrentState 사용)
            updateMapWithCurrentState();
            
            std::cout << "✅ 매핑 완료 - 최종 맵 생성됨" << std::endl;
            
            return "DONEMAPPING";
        }
        
        else if(msg_s.rfind("featureShot/", 0) == 0){
            
            std::string featureName = msg_s.substr(std::string("featureShot/").length());
            FeaturePoint currentFeature;
            currentFeature.position = currentLocation;
            currentFeature.name = featureName;
            featureLocations.push_back(currentFeature);            
            
            updateMapWithCurrentState();
            
            return "FEATURESHOTOK";
        }

        else{
            std::cout << "Invalid mapping command: " << msg << std::endl;
            return "INVALID";
        }
    }

    else{
        std::cout << "Mapping is already done." << std::endl;
        return "MAPPINGDONE";
    }
}

std::vector<std::pair<int, int>> Mapper::findSearchingPathBFS(std::pair<int,int>end)
{
	int ysize = map.size();
	int xsize = map[0].size();

	int dy[] {1, -1, 0, 0};
	int dx[]{ 0, 0, -1, 1 };

    std::pair<int, int> start = currentLocation;
	std::vector<std::vector<bool>> visited(ysize, std::vector<bool>(xsize, false));
	std::vector<std::pair<int, int>> result;
	std::vector<std::vector<std::pair<int, int>>> parent(ysize, std::vector<std::pair<int, int>>(xsize, { -1,-1 }));

	std::queue<std::pair<int, int>> q;
	q.push(start);
	visited[start.first][start.second] = true; 

	while (!q.empty())
	{
		std::pair<int, int> cur = q.front();
		q.pop();

		// 목적지에 도착하면 탐색 종료
		if (cur.first == end.first && cur.second == end.second) break;
		
		for (int i = 0; i < 4; i++) {
			int ny = cur.first + dy[i];
			int nx = cur.second + dx[i];

			// 범위 체크
			if (ny < 0 || nx < 0 || ny >= ysize || nx >= xsize) continue;
			if (map[ny][nx] < 0) continue;
			if (visited[ny][nx]) continue;

			// 방문 처리 및 부모 기록
			visited[ny][nx] = true;
			parent[ny][nx] = { cur.first, cur.second };
			q.push({ ny, nx });
		}
	}
	if ((parent[end.first][end.second].first != -1 || 
	     parent[end.first][end.second].second != -1) ||
	    (end.first == start.first && end.second == start.second)) {
		
		int cy = end.first, cx = end.second;
		while (cy != -1 && cx != -1) {
			result.push_back({ cy, cx });
			if (cy == start.first && cx == start.second) break;

			auto p = parent[cy][cx];
			cy = p.first;
			cx = p.second;
		}
		std::reverse(result.begin(), result.end());
		return result;
	}

	else {
		result.clear();
		return result;
	}
}

std::vector<std::pair<int, int>> Mapper::findNavigatingPathBFS(std::pair<int,int>end)
{
	int ysize = map.size();
	int xsize = map[0].size();

	int dy[] {1, -1, 0, 0};
	int dx[]{ 0, 0, -1, 1 };

    std::pair<int, int> start = currentLocation;


	std::vector<std::vector<bool>> visited(ysize, std::vector<bool>(xsize, false));
	std::vector<std::pair<int, int>> result;
	std::vector<std::vector<std::pair<int, int>>> parent(ysize, std::vector<std::pair<int, int>>(xsize, { -1,-1 }));

	std::queue<std::pair<int, int>> q;
	q.push(start);
	visited[start.first][start.second] = true; 

	while (!q.empty())
	{
		std::pair<int, int> cur = q.front();
		q.pop();

		// 목적지에 도착하면 탐색 종료
		if (cur.first == end.first && cur.second == end.second) break;
		
		for (int i = 0; i < 4; i++) {
			int ny = cur.first + dy[i];
			int nx = cur.second + dx[i];

			// 범위 체크
			if (ny < 0 || nx < 0 || ny >= ysize || nx >= xsize) continue;
			if (map[ny][nx] < 1) continue;
			if (visited[ny][nx]) continue;

			// 방문 처리 및 부모 기록
			visited[ny][nx] = true;
			parent[ny][nx] = { cur.first, cur.second };
			q.push({ ny, nx });
		}
	}
	if ((parent[end.first][end.second].first != -1 || 
	     parent[end.first][end.second].second != -1) ||
	    (end.first == start.first && end.second == start.second)) {
		
		int cy = end.first, cx = end.second;
		while (cy != -1 && cx != -1) {
			result.push_back({ cy, cx });
			if (cy == start.first && cx == start.second) break;

			auto p = parent[cy][cx];
			cy = p.first;
			cx = p.second;
		}
		std::reverse(result.begin(), result.end());
		return result;
	}

	else {
		result.clear();
		return result;
	}
}

void Mapper::updateNavigateResult(std::pair<int,int> locate, int state)
{
    map[locate.first][locate.second] = state;
}

bool Mapper::IsMappingDone()
{
    return doneMapping;
}


// 실시간 맵 업데이트 함수
void Mapper::updateMapWithCurrentState()
{
    if(locations.empty()) {
        return;
    }
    
    std::vector<std::pair<int, int>> tempLocations;
    std::vector<FeaturePoint> tempFeatureLocations;

    //std::vector<std::pair<int, int>> tempFeatureLocations;
    
    // locations 좌표 보정
    for(const auto& loc : locations) {
        tempLocations.push_back({
            loc.first + (mostDown * -1),   // y 좌표 보정
            loc.second + (mostLeft * -1)   // x 좌표 보정
        });
    }
    
    // featureLocations 좌표 보정
    for(const auto& feature : featureLocations) {
        tempFeatureLocations.push_back({
            {feature.position.first + (mostDown * -1), feature.position.second + (mostLeft * -1)},
            feature.name
        });
    }
    
    // 보정된 좌표들에서 최대/최소 x, y 좌표 찾기
    int maxX = 0, maxY = 0;
    int minX = 0, minY = 0;
    
    if(!tempLocations.empty()) {
        maxX = tempLocations[0].second;
        maxY = tempLocations[0].first;
        minX = tempLocations[0].second;
        minY = tempLocations[0].first;
    }
    
    for(const auto& loc : tempLocations) {
        if(loc.second > maxX) maxX = loc.second;
        if(loc.first > maxY) maxY = loc.first;
        if(loc.second < minX) minX = loc.second;
        if(loc.first < minY) minY = loc.first;
    }
    for(const auto& loc : tempFeatureLocations) {
        if(loc.position.second > maxX) maxX = loc.position.second;
        if(loc.position.first > maxY) maxY = loc.position.first;
        if(loc.position.second < minX) minX = loc.position.second;
        if(loc.position.first < minY) minY = loc.position.first;
    }

    // 현재 위치도 포함하여 범위 확인
    int currentY = currentLocation.first + (mostDown * -1);
    int currentX = currentLocation.second + (mostLeft * -1);
    if(currentX > maxX) maxX = currentX;
    if(currentY > maxY) maxY = currentY;
    if(currentX < minX) minX = currentX;
    if(currentY < minY) minY = currentY;

    // 맵 크기 설정 (최소값이 0이 되도록 조정)
    int mapWidth = maxX - minX + 1;
    int mapHeight = maxY - minY + 1;

    sendFeatureinfo = tempFeatureLocations;

    // 맵 초기화
    map.clear();
    map.resize(mapHeight, std::vector<int>(mapWidth, 0));

    // 보정된 locations에 있는 좌표들에 1 할당 (이동 경로)
    for(const auto& loc : tempLocations) {
        int x = loc.second - minX;  // 최소값 기준으로 재조정
        int y = loc.first - minY;
        if(x >= 0 && x < mapWidth && y >= 0 && y < mapHeight) {
            map[y][x] = 1;
        }
    }

    // 보정된 featureLocations에 있는 좌표들에 2 할당 (특징점)
    for(const auto& loc : tempFeatureLocations) {
        int x = loc.position.second - minX;  // 최소값 기준으로 재조정
        int y = loc.position.first - minY;
        if(x >= 0 && x < mapWidth && y >= 0 && y < mapHeight) {
            map[y][x] = 2;
        }
    }

    // 현재 위치 표시
    int finalCurrentX = currentX - minX;
    int finalCurrentY = currentY - minY;
    
    if(finalCurrentY >= 0 && finalCurrentY < mapHeight && 
       finalCurrentX >= 0 && finalCurrentX < mapWidth) {
        map[finalCurrentY][finalCurrentX] = 3;
    }
}