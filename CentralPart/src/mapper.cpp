module;

#include <cstring>
#include <iostream>
#include <vector>
#include <queue>
#include<algorithm>
#include <string>
#include <climits>

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
    
    // 매핑이 완료된 후에는 위치 업데이트를 하지 않음
    if(doneMapping) {
        std::cout << "⚠️  매핑 완료 후 getMappingMessages 호출됨 (위치 업데이트 스킵): " << msg << std::endl;
        return "MAPPINGDONE";
    }
    
    if(strcmp(msg, "left") == 0){
        currentLocation.second -= 1;
        if(currentLocation.second < mostLeft){
            mostLeft = currentLocation.second;
        }
        locations.push_back(currentLocation);
        lastOrientation = "left";
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
        lastOrientation = "right";
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
        lastOrientation = "up";
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
        lastOrientation = "down";
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
        currentFeature.orientation = lastOrientation;
        featureLocations.push_back(currentFeature);            
        
        updateMapWithCurrentState();
        
        return "FEATURESHOTOK";
    }

    else{
        std::cout << "Invalid mapping command: " << msg << std::endl;
        return "INVALID";
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

std::vector<std::pair<int, int>> Mapper::findNavigatingPathBFS(std::string featureName)
{
    std::pair<int,int> end = getPositionByName(featureName);
    
    // 특징점을 찾지 못한 경우 빈 결과 반환
    if(end.first == -1 && end.second == -1) {
        std::cout << "Mapper: 특징점 '" << featureName << "' 을(를) 찾을 수 없습니다." << std::endl;
        return std::vector<std::pair<int, int>>();
    }

    // 맵이 초기화되지 않은 경우 체크
    if(map.empty()) {
        std::cout << "❌ 맵이 비어있습니다. 매핑을 먼저 완료하세요." << std::endl;
        return std::vector<std::pair<int, int>>();
    }

	int ysize = map.size();
	int xsize = map[0].size();
	
	std::cout << "🗺️ BFS 시작: 맵 크기 " << ysize << "x" << xsize << std::endl;
	std::cout << "   시작: (" << currentLocation.first << ", " << currentLocation.second << ")" << std::endl;
	std::cout << "   목표: (" << end.first << ", " << end.second << ")" << std::endl;
	
	// 시작/목표 위치가 맵 범위 내에 있는지 확인
	if(currentLocation.first < 0 || currentLocation.first >= ysize ||
	   currentLocation.second < 0 || currentLocation.second >= xsize) {
	    std::cout << "❌ 시작 위치가 맵 범위를 벗어났습니다." << std::endl;
	    return std::vector<std::pair<int, int>>();
	}
	
	if(end.first < 0 || end.first >= ysize ||
	   end.second < 0 || end.second >= xsize) {
	    std::cout << "❌ 목표 위치가 맵 범위를 벗어났습니다." << std::endl;
	    return std::vector<std::pair<int, int>>();
	}
	
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
		std::cout << "✅ BFS 성공: " << result.size() << "개 지점 경로 찾음" << std::endl;
		return result;
	}

	else {
		std::cout << "❌ BFS 실패: 목표 (" << end.first << ", " << end.second << ")에 도달할 수 없음" << std::endl;
		result.clear();
		return result;
	}
}

void Mapper::updateSearchingResult(std::pair<int,int> locate, int state)
{
    // 특징점(2)이 있는 위치는 덮어쓰지 않음
    if(map[locate.first][locate.second] != 2) {
        map[locate.first][locate.second] = state;
    }
    currentLocation = locate;  // 현재 위치 업데이트
    std::cout << "📍 Mapper 현재 위치 업데이트: (" << currentLocation.first << ", " << currentLocation.second << ")" << std::endl;
}

bool Mapper::IsMappingDone()
{
    return doneMapping;
}


// 방문한 특징점을 제외하고 가장 가까운 특징점 찾기
std::vector<std::pair<int,int>> Mapper::moveToNearestFeaturePoint(const std::vector<std::string>& visitedFeatures)
{

    // 특징점이 없으면 빈 경로 반환
    if(sendFeatureinfo.empty())
    {
        std::cout << "❌ 등록된 특징점이 없습니다." << std::endl;
        return std::vector<std::pair<int,int>>();
    }
    
    // 가장 가까운 특징점 찾기 (헤밀턴 거리 기준, 방문한 특징점 제외)
    int minDistance = INT_MAX;
    std::pair<int,int> nearestFeature = {-1, -1};
    std::string nearestFeatureName = "";
    
    for(const auto& feature : sendFeatureinfo)
    {
        // 이미 방문한 특징점은 스킵
        bool alreadyVisited = false;
        for(const auto& visited : visitedFeatures) {
            if(feature.name == visited) {
                alreadyVisited = true;
                break;
            }
        }
        
        if(alreadyVisited) {
            std::cout << "⏭️  특징점 '" << feature.name << "' 이미 방문함, 스킵" << std::endl;
            continue;
        }
        
        // 헤밀턴 거리 계산: |x1 - x2| + |y1 - y2|
        int distance = abs(currentLocation.first - feature.position.first) + 
                      abs(currentLocation.second - feature.position.second);
        
        if(distance < minDistance)
        {
            minDistance = distance;
            nearestFeature = feature.position;
            nearestFeatureName = feature.name;
        }
    }
    
    // 모든 특징점을 방문했으면 빈 경로 반환
    if(nearestFeatureName.empty()) {
        return std::vector<std::pair<int,int>>();
    }
    return findNavigatingPathBFS(nearestFeatureName);
}

bool Mapper::AmIFeaturePoint()
{
    for(const auto& feature : sendFeatureinfo){
        if(feature.position == currentLocation){
            return true;
        }
    }
    return false;
} 
std::string Mapper::getDirection(std::pair<int,int> start, std::pair<int,int> end)
{
    if(start.first - end.first == 1 && start.second == end.second)
    {
        return "down";
    }
    else if(start.first - end.first == -1 && start.second == end.second)
    {
        return "up";
    }
    else if(start.first == end.first && start.second - end.second == 1)
    {
        return "left";
    }
    else if(start.first == end.first && start.second - end.second == -1)
    {
        return "right";
    }
    else
    {
        return "invalid";
    }
}

int Mapper::getIndexOfFeatureByName(const std::string& name) const
{
    std::cout << "🔍 getIndexOfFeatureByName 호출 - 찾는 이름: '" << name << "'" << std::endl;
    std::cout << "📋 전체 특징점 개수: " << featureLocations.size() << std::endl;
    
    for(size_t i = 0; i < featureLocations.size(); i++) {
        std::cout << "  [" << i << "] '" << featureLocations[i].name << "'";
        if(featureLocations[i].name == name) {
            std::cout << " ✅ 매칭!" << std::endl;
            return static_cast<int>(i);
        }
        std::cout << std::endl;
    }
    
    std::cerr << "❌ 특징점을 찾을 수 없음: '" << name << "'" << std::endl;
    return -1; 
}

std::pair<int, int> Mapper::getPositionByName(const std::string& name) const
{
    std::cout << "🔍 getPositionByName 호출됨 - 찾는 이름: '" << name << "' (길이: " << name.length() << ")" << std::endl;
    std::cout << "📍 등록된 특징점 목록 (" << sendFeatureinfo.size() << "개):" << std::endl;
    
    // sendFeatureinfo에서 검색 (항상 보정된 좌표 사용)
    for(size_t i = 0; i < sendFeatureinfo.size(); i++) {
        const auto& feature = sendFeatureinfo[i];
        std::cout << "  [" << i << "] 이름: '" << feature.name << "' (길이: " << feature.name.length() 
                  << ") 위치: (" << feature.position.first << ", " << feature.position.second << ")";
        
        // 문자 단위 비교
        bool match = (feature.name == name);
        std::cout << " - 일치: " << (match ? "✅" : "❌") << std::endl;
        
        if(match) {
            std::cout << "✅ 특징점 찾음: '" << name << "' at (" << feature.position.first << ", " << feature.position.second << ")" << std::endl;
            return feature.position;
        }
    }
    
    std::cout << "❌ 특징점 '" << name << "' 을(를) 찾을 수 없습니다." << std::endl;
    // 찾지 못한 경우 {-1, -1} 반환
    return {-1, -1};
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
    
    // doneMapping 완료 후에는 이미 보정된 좌표 사용
    if(doneMapping) {
        // 이미 보정된 좌표를 그대로 사용
        tempLocations = locations;
        tempFeatureLocations = featureLocations;
    } else {
        // 매핑 중에는 실시간으로 좌표 보정
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
    int currentY, currentX;
    if(doneMapping) {
        // 이미 보정된 현재 위치 사용
        currentY = currentLocation.first;
        currentX = currentLocation.second;
    } else {
        // 매핑 중에는 실시간 보정
        currentY = currentLocation.first + (mostDown * -1);
        currentX = currentLocation.second + (mostLeft * -1);
    }
    
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