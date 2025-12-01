// 맵 표시 관련 JavaScript

// 맵 표시 함수
function showMap(itemName, location) {
    const modal = document.getElementById('mapModal');
    const modalTitle = document.getElementById('modalTitle');
    const mapDisplay = document.getElementById('mapDisplay');
    
    modalTitle.textContent = `${itemName} 위치 정보 (${location})`;
    mapDisplay.innerHTML = '맵을 불러오는 중...';
    
    modal.style.display = 'block';
    
    // 맵 데이터 가져오기
    fetchMapForLocation(location);
}

// 맵 모달 닫기
function closeMapModal() {
    const modal = document.getElementById('mapModal');
    modal.style.display = 'none';
}

// 모달 외부 클릭시 닫기
window.onclick = function(event) {
    const modal = document.getElementById('mapModal');
    if (event.target === modal) {
        modal.style.display = 'none';
    }
}

// 특정 위치의 맵 데이터 가져오기
async function fetchMapForLocation(location) {
    try {
        const response = await fetch('/get-map');
        const data = await response.json();
        
        console.log('받은 맵 데이터:', data);
        
        if (data && data.status === 'success' && data.map) {
            displayInventoryMap(data, location);
        } else if (data && data.status === 'no_map') {
            document.getElementById('mapDisplay').innerHTML = 
                '<div style="color: orange;">⚠️ 맵이 아직 준비되지 않았습니다.</div>';
        } else {
            console.error('맵 데이터 구조 오류:', data);
            document.getElementById('mapDisplay').innerHTML = 
                `<div style="color: red;">❌ 맵 데이터 구조가 올바르지 않습니다.<br>받은 데이터: ${JSON.stringify(data)}</div>`;
        }
    } catch (error) {
        console.error('맵 데이터 로드 실패:', error);
        document.getElementById('mapDisplay').innerHTML = 
            `<div style="color: red;">❌ 맵 데이터를 불러올 수 없습니다.<br>오류: ${error.message}</div>`;
    }
}

// 재고 관리용 맵 표시 함수
function displayInventoryMap(mapData, targetLocation) {
    const mapDisplay = document.getElementById('mapDisplay');
    
    // 데이터 구조 확인
    let mapArray = mapData.map;
    let featuresArray = [];
    
    if (mapArray && mapArray.grid) {
        featuresArray = mapArray.features || [];
        mapArray = mapArray.grid;
    }
    
    if (!mapArray || !Array.isArray(mapArray) || mapArray.length === 0) {
        mapDisplay.innerHTML = '<div style="color: red;">🗺️ 맵 데이터가 없습니다.</div>';
        return;
    }
    
    const height = mapArray.length;
    const width = mapArray[0].length;
    
    // 특징점 위치 및 이름 매핑
    const featureMap = new Map();
    let targetFeaturePos = null;
    
    for (let i = 0; i < featuresArray.length; i++) {
        const feature = featuresArray[i];
        const key = `${feature.x},${feature.y}`;
        featureMap.set(key, {
            name: feature.name,
            number: i + 1,
            x: feature.x,
            y: feature.y
        });
        
        // 목표 위치 찾기
        if (feature.name === targetLocation) {
            targetFeaturePos = {x: feature.x, y: feature.y};
        }
    }
    
    // 로봇 현재 위치 찾기
    let robotPos = null;
    for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
            if (mapArray[y][x] === 3) {
                robotPos = {x: x, y: y};
                break;
            }
        }
        if (robotPos) break;
    }
    
    // Grid 생성
    let mapHtml = `
        <div style="
            display: grid;
            grid-template-columns: repeat(${width}, 80px);
            grid-template-rows: repeat(${height}, 80px);
            gap: 1px;
            justify-content: center;
            margin: 15px;
            max-width: 95%;
            overflow: auto;
        ">
    `;
    
    // 위에서 아래로 표시
    for (let y = height - 1; y >= 0; y--) {
        for (let x = 0; x < width; x++) {
            const value = mapArray[y][x];
            const posKey = `${x},${y}`;
            const feature = featureMap.get(posKey);
            
            // 로봇이 이 위치에 있는지 확인
            const hasRobot = robotPos && robotPos.x === x && robotPos.y === y;
            
            // 목표 재고 위치인지 확인
            const isTargetLocation = targetFeaturePos && 
                                    targetFeaturePos.x === x && 
                                    targetFeaturePos.y === y;
            
            let content = '';
            let style = 'width: 80px; height: 80px; display: flex; align-items: center; justify-content: center; font-size: 56px; position: relative; border-radius: 8px; overflow: visible;';
            
            // 셀 배경색 및 내용 결정
            if (value === 0) {
                content = ''; 
                style += ' background: #f0f2f5; border: 2px solid #e1e4e8;';
            } else if (value === 1) {
                content = ''; 
                style += ' background: #d4edda; border: 2px solid #c3e6cb;';
            } else if (value === -1) {
                content = ''; 
                style += ' background: #4a5568; border: 2px solid #2d3748;';
            } else if (value === 2 || value === 3 || feature) {
                // 특징점 또는 로봇이 있는 경우
                
                // 목표 위치 강조 표시
                if (isTargetLocation) {
                    style += ' background: #ffeb3b; border: 5px solid #ff5722; box-shadow: 0 0 20px rgba(255, 87, 34, 0.8);';
                } else {
                    style += ' background: #fff3cd; border: 2px solid #ffeeba;';
                }
                
                // 로봇과 특징점이 같은 위치에 있는 경우
                if (hasRobot && feature) {
                    content = `
                        <div style="display: flex; gap: 6px; align-items: center; font-size: 40px;">
                            <span>🤖</span>
                            <span>🔶</span>
                        </div>
                    `;
                } 
                // 로봇만 있는 경우
                else if (hasRobot) {
                    content = '🤖';
                    style += ' background: #cce5ff; border: 2px solid #b8daff;';
                }
                // 특징점만 있는 경우
                else if (feature) {
                    content = '🔶';
                }
                
                // 목표 위치 라벨 추가
                if (isTargetLocation) {
                    content += `<span style="
                        position: absolute;
                        bottom: -10px;
                        left: 50%;
                        transform: translateX(-50%);
                        background: #ff5722;
                        color: white;
                        padding: 4px 12px;
                        border-radius: 12px;
                        font-size: 20px;
                        font-weight: bold;
                        white-space: nowrap;
                        box-shadow: 0 2px 8px rgba(0,0,0,0.3);
                    ">📍 ${targetLocation}</span>`;
                }
            } else {
                content = value;
                style += ' background: #fff;';
            }
            
            mapHtml += `<div style="${style}">${content}</div>`;
        }
    }
    
    mapHtml += '</div>';
    
    // 범례 추가
    if (targetFeaturePos) {
        const isRobotAtTarget = robotPos && robotPos.x === targetFeaturePos.x && robotPos.y === targetFeaturePos.y;
        
        mapHtml += `
            <div style="margin-top: 20px; padding: 15px; background: #f8f9fa; border-radius: 10px; text-align: center;">
                <p style="font-size: 18px; font-weight: bold; color: #333; margin: 0;">
                    <span style="color: #ff5722;">${targetLocation}</span> 제품은 노란색 위치에 있습니다.
                </p>
                ${isRobotAtTarget ? 
                    '<p style="font-size: 16px; color: #4caf50; margin: 5px 0 0 0;">✅ 로봇이 해당 위치에 있습니다!</p>' : 
                    `<p style="font-size: 16px; color: #666; margin: 5px 0 0 0;">🤖 로봇은 다른 위치에 있습니다.</p>
                    <button onclick="moveRobotToLocation('${targetLocation}', ${targetFeaturePos.x}, ${targetFeaturePos.y})" 
                        style="
                            margin-top: 15px;
                            background: linear-gradient(145deg, #667eea, #764ba2);
                            color: white;
                            border: none;
                            padding: 12px 30px;
                            border-radius: 25px;
                            font-size: 16px;
                            font-weight: bold;
                            cursor: pointer;
                            box-shadow: 0 4px 15px rgba(102, 126, 234, 0.3);
                            transition: all 0.3s ease;
                        "
                        onmouseover="this.style.transform='translateY(-2px)'; this.style.boxShadow='0 6px 20px rgba(102, 126, 234, 0.4)'"
                        onmouseout="this.style.transform='translateY(0)'; this.style.boxShadow='0 4px 15px rgba(102, 126, 234, 0.3)'"
                    >
                        🚀 해당 위치로 이동
                    </button>`
                }
            </div>
        `;
    }
    
    mapDisplay.innerHTML = mapHtml;
}

// 로봇을 특정 위치로 이동시키는 함수
async function moveRobotToLocation(locationName, targetX, targetY) {
    if (!confirm(`로봇을 "${locationName}" 위치로 이동시키겠습니까?`)) {
        return;
    }
    
    try {
        const response = await fetch('/move-to', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ 
                location: locationName
            })
        });
        
        const data = await response.json();
        
        if (data.status === 'success') {
            alert(`✅ 로봇이 "${locationName}" 위치로 이동을 시작합니다!`);
            // 맵 새로고침
            setTimeout(() => {
                const location = document.getElementById('modalTitle').textContent.match(/\((.+)\)/)?.[1];
                if (location) fetchMapForLocation(location);
            }, 1000);
        } else {
            alert(`❌ 이동 실패: ${data.message}`);
        }
    } catch (error) {
        console.error('이동 요청 오류:', error);
        alert('❌ 서버 통신 오류가 발생했습니다.');
    }
}
