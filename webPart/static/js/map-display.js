// 맵 표시 관련 JavaScript

// 실시간 업데이트 관련 변수
let mapUpdateInterval = null;
const MAP_UPDATE_INTERVAL = 1000; // 1초마다 업데이트
let currentLocation = null; // 현재 표시 중인 위치

// 맵 표시 함수
function showMap(itemName, location) {
    const modal = document.getElementById('mapModal');
    const modalTitle = document.getElementById('modalTitle');
    const mapDisplay = document.getElementById('mapDisplay');
    
    console.log('🗺️ 맵 모달 열기:', itemName, location);
    
    modalTitle.textContent = `${itemName} 위치 정보 (${location})`;
    mapDisplay.innerHTML = '맵을 불러오는 중...';
    
    modal.style.display = 'block';
    
    // 현재 위치 저장
    currentLocation = location;
    
    // 맵 데이터 가져오기 (초기 로드)
    fetchMapForLocation(location);
    
    // 실시간 업데이트 시작
    if (mapUpdateInterval) {
        clearInterval(mapUpdateInterval);
    }
    console.log('⏱️ 실시간 업데이트 시작 (1초 간격)');
    mapUpdateInterval = setInterval(() => {
        if (currentLocation) {
            fetchMapForLocation(currentLocation);
        }
    }, MAP_UPDATE_INTERVAL);
}

// 맵 모달 닫기
function closeMapModal() {
    console.log('🚪 맵 모달 닫기');
    const modal = document.getElementById('mapModal');
    modal.style.display = 'none';
    
    // 실시간 업데이트 중지
    if (mapUpdateInterval) {
        console.log('⏹️ 실시간 업데이트 중지');
        clearInterval(mapUpdateInterval);
        mapUpdateInterval = null;
    }
    
    currentLocation = null;
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
        console.log('🔄 맵 데이터 요청 중...', new Date().toLocaleTimeString());
        const response = await fetch('/get-map');
        const data = await response.json();
        
        console.log('📥 맵 데이터 수신:', data.status);
        
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
    
    console.log('🔍 Feature 검색 시작 - targetLocation:', targetLocation);
    console.log('📍 Features 배열:', featuresArray);
    
    for (let i = 0; i < featuresArray.length; i++) {
        const feature = featuresArray[i];
        const key = `${feature.x},${feature.y}`;
        featureMap.set(key, {
            name: feature.name,
            number: i + 1,
            x: feature.x,
            y: feature.y
        });
        
        console.log(`   - Feature ${i + 1}: "${feature.name}" vs "${targetLocation}"`);
        
        // 목표 위치 찾기 (공백 제거 후 대소문자 무시 비교)
        if (feature.name && targetLocation && 
            feature.name.trim().toLowerCase() === targetLocation.trim().toLowerCase()) {
            targetFeaturePos = {x: feature.x, y: feature.y, name: feature.name};
            console.log('✅ 목표 위치 찾음!', targetFeaturePos);
        }
    }
    
    if (!targetFeaturePos) {
        console.warn('⚠️ 목표 위치를 찾지 못했습니다. targetLocation:', targetLocation);
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
    
    // 반응형 셀 크기 계산: modal/mapDisplay 크기 기준으로 자동 조정
    const container = mapDisplay;
    const containerStyle = window.getComputedStyle(container);
    const paddingLeft = parseInt(containerStyle.paddingLeft) || 0;
    const paddingRight = parseInt(containerStyle.paddingRight) || 0;
    const paddingTop = parseInt(containerStyle.paddingTop) || 0;
    const paddingBottom = parseInt(containerStyle.paddingBottom) || 0;

    // 사용 가능한 가로/세로 픽셀 계산
    const availableWidth = Math.max(container.clientWidth - paddingLeft - paddingRight - 40, 100);
    // 높이는 화면 높이의 일부로 제한 (모달 내부 여유 공간 고려)
    const viewportHeight = Math.max(window.innerHeight - 200, 200);
    const availableHeight = Math.max(Math.min(container.clientHeight || viewportHeight, viewportHeight) - paddingTop - paddingBottom - 120, 100);

    // 셀 크기(최대 80px, 최소 18px)
    const cellSizeByWidth = Math.floor(availableWidth / width);
    const cellSizeByHeight = Math.floor(availableHeight / height);
    let cellSize = Math.max(18, Math.min(80, Math.min(cellSizeByWidth, cellSizeByHeight)));

    // 작은 화면에서는 셀 사이즈를 더 작게 하여 가독성 확보
    if (window.innerWidth <= 480) {
        cellSize = Math.max(18, Math.min(cellSize, 44));
    } else if (window.innerWidth <= 768) {
        cellSize = Math.max(24, Math.min(cellSize, 64));
    }

    // Grid 생성
    let mapHtml = `
        <div style="
            display: grid;
            grid-template-columns: repeat(${width}, ${cellSize}px);
            grid-template-rows: repeat(${height}, ${cellSize}px);
            gap: 6px;
            justify-content: center;
            margin: 8px auto;
            max-width: 100%;
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
            const fontSizeLarge = Math.max(12, Math.floor(cellSize * 0.7));
            const fontSizeSmall = Math.max(10, Math.floor(cellSize * 0.35));
            let style = `width: ${cellSize}px; height: ${cellSize}px; display: flex; align-items: center; justify-content: center; font-size: ${fontSizeLarge}px; position: relative; border-radius: ${Math.max(6, Math.floor(cellSize * 0.12))}px; overflow: visible;`;
            
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
                        <div style="display: flex; gap: ${Math.max(4, Math.floor(cellSize*0.08))}px; align-items: center; font-size: ${Math.max(18, Math.floor(cellSize*0.5))}px;">
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
                    const labelFont = Math.max(12, Math.floor(cellSize * 0.35));
                    content += `<span style="
                        position: absolute;
                        bottom: -10px;
                        left: 50%;
                        transform: translateX(-50%);
                        background: #ff5722;
                        color: white;
                        padding: 4px 10px;
                        border-radius: 10px;
                        font-size: ${labelFont}px;
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

    // 범례/액션 박스 생성 (아래에 배치)
    let actionHtml = '';
    console.log('🎯 targetFeaturePos 확인:', targetFeaturePos);
    console.log('🤖 robotPos 확인:', robotPos);
    
    if (targetFeaturePos) {
        const isRobotAtTarget = robotPos && robotPos.x === targetFeaturePos.x && robotPos.y === targetFeaturePos.y;
        console.log('✅ 액션 박스 생성 중... isRobotAtTarget:', isRobotAtTarget);

        actionHtml = `
            <div style="width: 100%; max-width: 820px; box-sizing: border-box; padding: 12px; background: #f8f9fa; border-radius: 10px; text-align: center;">
                <p style="font-size: 18px; font-weight: bold; color: #333; margin: 0;">
                    <span style="color: #ff5722;">${targetLocation}</span> 제품은 노란색 위치에 있습니다.
                </p>
                ${isRobotAtTarget ? 
                    '<p style="font-size: 16px; color: #4caf50; margin: 8px 0 0 0;">✅ 로봇이 해당 위치에 있습니다!</p>' : 
                    `<p style="font-size: 16px; color: #666; margin: 8px 0 0 0;">🤖 로봇은 다른 위치에 있습니다.</p>
                    <div style="margin-top:12px; display:flex; justify-content:center; gap:10px; flex-wrap:wrap;">
                        <button onclick="moveRobotToLocation('${targetLocation}', ${targetFeaturePos.x}, ${targetFeaturePos.y})" 
                            style="
                                background: linear-gradient(145deg, #667eea, #764ba2);
                                color: white;
                                border: none;
                                padding: 12px 20px;
                                border-radius: 20px;
                                font-size: 15px;
                                font-weight: bold;
                                cursor: pointer;
                                box-shadow: 0 4px 12px rgba(102, 126, 234, 0.25);
                                transition: all 0.18s ease;
                                min-width: 140px;
                            "
                            onmouseover="this.style.transform='translateY(-2px)'; this.style.boxShadow='0 6px 20px rgba(102, 126, 234, 0.35)'"
                            onmouseout="this.style.transform='translateY(0)'; this.style.boxShadow='0 4px 12px rgba(102, 126, 234, 0.25)'"
                        >
                            🚀 해당 위치로 이동
                        </button>
                        <button onclick="closeMapModal()" style="background:#e0e0e0; border:none; padding:12px 20px; border-radius:20px; font-size:15px;">닫기</button>
                    </div>`
                }
            </div>
        `;
    } else {
        console.warn('⚠️ targetFeaturePos가 null이어서 액션 박스를 생성하지 않습니다.');
        console.warn('   - targetLocation:', targetLocation);
        console.warn('   - features 배열 길이:', featuresArray.length);
        actionHtml = `
            <div style="width: 100%; max-width: 820px; box-sizing: border-box; padding: 12px; background: #fff3cd; border-radius: 10px; text-align: center; border: 2px solid #ffc107;">
                <p style="font-size: 16px; color: #856404; margin: 0;">
                    ⚠️ "<strong>${targetLocation}</strong>" 위치를 맵에서 찾을 수 없습니다.
                </p>
                <p style="font-size: 14px; color: #856404; margin: 8px 0 0 0;">
                    해당 특징점이 등록되어 있는지 확인해주세요.
                </p>
            </div>
        `;
    }

    // 전체를 세로로 배치: 맵(위) + 액션 박스(아래)
    const finalHtml = `
        <div style="display:flex; flex-direction:column; align-items:center; gap:12px;">
            ${mapHtml}
            ${actionHtml}
        </div>
    `;

    mapDisplay.innerHTML = finalHtml;
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
        
        if (data.status === 'completed') {
            // 이동 완료 후 Searching 모드 전환 확인 팝업
            if (confirm(`✅ 로봇이 "${locationName}" 위치로 이동을 완료했습니다! (${data.completed_steps}단계)\n\nSearching 모드로 전환하시겠습니까?`)) {
                try {
                    const navDoneResponse = await fetch('/navigatedone', {
                        method: 'POST',
                        headers: {
                            'Content-Type': 'application/json'
                        }
                    });
                    
                    const navDoneData = await navDoneResponse.json();
                    
                    if (navDoneData.status === 'success') {
                        alert(`🔍 ${navDoneData.message}\n로봇이 자동으로 특징점을 탐색합니다.`);
                    } else {
                        alert(`⚠️ Searching 모드 전환 실패: ${navDoneData.message}`);
                    }
                } catch (error) {
                    console.error('Searching 모드 전환 오류:', error);
                    alert('❌ Searching 모드 전환 중 오류가 발생했습니다.');
                }
            }
            
            // 맵 새로고침
            setTimeout(() => {
                const location = document.getElementById('modalTitle').textContent.match(/\((.+)\)/)?.[1];
                if (location) fetchMapForLocation(location);
            }, 1000);
        } else if (data.status === 'failed') {
            alert(`❌ 이동 실패: ${data.error || data.message}`);
        } else {
            alert(`❌ 이동 실패: ${data.message}`);
        }
    } catch (error) {
        console.error('이동 요청 오류:', error);
        alert('❌ 서버 통신 오류가 발생했습니다.');
    }
}
