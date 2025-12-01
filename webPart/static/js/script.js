/* static/js/script.js */

// ==========================================
// 1. 전역 변수 및 상수 설정
// ==========================================
const directionButtons = document.querySelectorAll('.direction-btn');
const mappingBtn = document.getElementById('mapping-btn');
const remappingBtn = document.getElementById('remapping-btn');
const cameraBtn = document.getElementById('camera-btn');
const mapGrid = document.getElementById('map-grid');
const mapInfo = document.getElementById('map-info');

// 팝업 관련 요소
const featurePopup = document.getElementById('feature-name-popup');
const nameInput = document.getElementById('feature-name-input');
const confirmBtn = document.getElementById('confirm-feature-btn');
const cancelBtn = document.getElementById('cancel-feature-btn');
const mappingDonePopup = document.getElementById('mapping-done-popup');
const closeMappingPopupBtn = document.getElementById('close-mapping-popup');

let mapUpdateInterval = null;
let isMappingCompleted = false;
// Poll intervals (milliseconds)
const MAP_POLL_INTERVAL_ACTIVE = 3000; // normal active polling (3s)
const MAP_POLL_INTERVAL_IDLE = 5000;   // when mapping completed / idle (5s)

// ==========================================
// 2. 핵심 로직: 맵 렌더링 및 데이터 가져오기
// ==========================================

// 맵 그리기 함수
function displayMap(mapData) {
    if (!mapData || !Array.isArray(mapData) || mapData.length === 0) {
        mapGrid.innerHTML = '<div class="map-status">🗺️ 맵 데이터가 없습니다.</div>';
        return;
    }
    
    // C++에서 오는 데이터는 [y][x] 형태이거나, 1차원 배열일 수 있음.
    // 여기서는 2차원 배열 [row][col]로 가정 (MainController 수정본 기준)
    const height = mapData.length;
    const width = mapData[0].length;
    
    // 특징점(값 2)의 순서를 매기기 위해 위치 수집
    // (화면 표시 기준: 왼쪽 아래 -> 오른쪽 위 순서로 번호 부여 예시)
    const featurePositions = [];
    for (let y = height - 1; y >= 0; y--) { 
        for (let x = 0; x < width; x++) {    
            if (mapData[y][x] === 2) {
                featurePositions.push({x: x, y: y, displayY: height - 1 - y});
            }
        }
    }
    
    // CSS Grid 레이아웃 업데이트
    mapGrid.style.gridTemplateColumns = `repeat(${width}, 1fr)`;
    mapGrid.style.gridTemplateRows = `repeat(${height}, 1fr)`;
    mapGrid.innerHTML = '';
    
    // 맵 데이터 순회하며 셀 생성 (위쪽 행부터 렌더링)
    for (let y = height-1; y >= 0; y--) {
        for (let x = 0; x < width; x++) {
            const cell = document.createElement('div');
            cell.className = 'map-cell';
            
            const value = mapData[y][x];
            
            // 값에 따른 스타일 및 아이콘 설정
            if (value === 0) {
                cell.classList.add('empty'); // 미지 영역
                // cell.textContent = '·'; 
            } else if (value === 1) {
                cell.classList.add('path'); // 이동 가능
                // cell.textContent = '';
            } else if (value === 2) {
                cell.classList.add('feature'); // 재고/특징점
                
                // 번호 찾기
                const featureIndex = featurePositions.findIndex(pos => pos.x === x && pos.y === y);
                const featureNumber = featureIndex + 1;
                cell.innerHTML = `🔶<span class="feature-number">${featureNumber}</span>`;
            } else if (value === 3) {
                cell.classList.add('current-position'); // 로봇
                cell.textContent = '🤖';
            } else if (value === -1) {
                cell.classList.add('blocked'); // 벽/장애물
                cell.textContent = '⬛';
            }
            
            // 디버깅용 좌표 툴팁
            cell.title = `(${x}, ${y}) Val: ${value}`;
            mapGrid.appendChild(cell);
        }
    }
    
    // 정보 업데이트
    if (mapInfo) {
        mapInfo.textContent = `맵 크기: ${width} × ${height} | 특징점: ${featurePositions.length}개`;
    }
}

// 서버에서 맵 데이터 가져오기
async function fetchMapData() {
    try {
        const response = await fetch('/get-map');
        const data = await response.json();
        
        if (data.status === 'success' && data.map) {
            // data.map이 {grid: [...], features: [...]} 형태일 경우 data.map.grid 사용
            // 2차원 배열 그대로 오는 경우 data.map 사용
            const gridData = data.map.grid ? data.map.grid : data.map;
            displayMap(gridData);
        } else if (data.status === 'no_map') {
            mapGrid.innerHTML = `<div class="map-status">🔄 ${data.message || '맵 준비 중...'}</div>`;
        } else {
            console.error('맵 데이터 오류:', data.message);
            // mapGrid.innerHTML = '<div class="map-status">⚠️ 데이터 오류</div>';
        }
    } catch (error) {
        console.error('네트워크 오류:', error);
        mapGrid.innerHTML = '<div class="map-status">🌐 연결 끊김</div>';
    }
}

// 주기적 업데이트 제어
function startMapUpdates() {
    /*
    fetchMapData(); // 즉시 실행
    if (!mapUpdateInterval) {
        mapUpdateInterval = setInterval(fetchMapData, 500); // 0.5초마다 갱신
    }
    */
    // Always perform an immediate fetch
    fetchMapData();

    // Clear any existing interval to avoid duplicates
    if (mapUpdateInterval) {
        clearInterval(mapUpdateInterval);
        mapUpdateInterval = null;
    }

    // Choose interval based on mapping state
    const interval = isMappingCompleted ? MAP_POLL_INTERVAL_IDLE : MAP_POLL_INTERVAL_ACTIVE;
    mapUpdateInterval = setInterval(fetchMapData, interval);
}

function stopMapUpdates() {
    if (mapUpdateInterval) {
        clearInterval(mapUpdateInterval);
        mapUpdateInterval = null;
    }
}

// ==========================================
// 3. 통신 요청 함수들
// ==========================================

// 카메라 촬영 요청 (이름 포함)
async function sendCameraRequest(name) {
    try {
        const response = await fetch('/camera', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ name: name })
        });
        const data = await response.json();
        
        if (data.status === 'success') {
            alert(`✅ 저장 완료: "${data.name}"`);
            fetchMapData(); // 맵 즉시 갱신
        } else {
            alert(`❌ 실패: ${data.message}`);
        }
    } catch (error) {
        console.error('카메라 요청 에러:', error);
        alert('서버 통신 오류가 발생했습니다.');
    }
}

// 방향 제어 요청
async function sendDirectionControl(direction) {
    try {
        const response = await fetch('/control', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ direction: direction })
        });
        const data = await response.json();

        if (data.status === 'mapping_done') {
            showMappingDonePopup();
        } else if (data.status === 'move_fail') {
            showObstaclePopup();
        } else if (data.status !== 'success') {
            console.error('이동 실패:', data.message);
        }
    } catch (error) {
        console.error('제어 요청 실패:', error);
    }
}

// ==========================================
// 4. UI 제어 및 팝업 함수
// ==========================================

function setMappingButtonToReady() {
    if (mappingBtn) {
        mappingBtn.setAttribute('data-state', 'ready');
        mappingBtn.querySelector('.mapping-text').textContent = '매핑 종료';
        mappingBtn.disabled = false;
        mappingBtn.style.opacity = '1';
        isMappingCompleted = false;
    }
}

function setMappingButtonToCompleted() {
    if (mappingBtn) {
        mappingBtn.setAttribute('data-state', 'completed');
        mappingBtn.querySelector('.mapping-text').textContent = '매핑 완료됨';
        mappingBtn.disabled = true;
        mappingBtn.style.opacity = '0.6';
        isMappingCompleted = true;
        // Update polling interval to idle mode
        startMapUpdates();
    }
}

function showMappingDonePopup() {
    if (mappingDonePopup) {
        mappingDonePopup.style.display = 'flex';
        setMappingButtonToCompleted();
    }
}

function hideMappingDonePopup() {
    if (mappingDonePopup) {
        mappingDonePopup.style.display = 'none';
    }
}

function showObstaclePopup() {
    const obstaclePopup = document.getElementById('obstacle-popup');
    if (obstaclePopup) {
        obstaclePopup.style.display = 'flex';
    }
}

function hideObstaclePopup() {
    const obstaclePopup = document.getElementById('obstacle-popup');
    if (obstaclePopup) {
        obstaclePopup.style.display = 'none';
    }
}

// ==========================================
// 5. 이벤트 리스너 통합 (DOMContentLoaded)
// ==========================================

document.addEventListener('DOMContentLoaded', () => {
    console.log('🚀 Script Loaded & Ready');

    // 1. 초기 상태 설정
    setMappingButtonToReady();
    startMapUpdates();

    // 2. 방향 버튼 이벤트
    directionButtons.forEach(button => {
        button.addEventListener('click', function() {
            const direction = this.getAttribute('data-direction');
            
            // 클릭 애니메이션 효과
            this.classList.add('clicked');
            setTimeout(() => this.classList.remove('clicked'), 200);

            sendDirectionControl(direction);
        });
    });

    // 3. 카메라 버튼 -> 팝업 열기
    if (cameraBtn) {
        cameraBtn.addEventListener('click', () => {
            if (featurePopup) {
                if (nameInput) nameInput.value = ''; // 초기화
                featurePopup.style.display = 'flex';
                if (nameInput) nameInput.focus();
            } else {
                // 팝업 HTML이 없는 경우 비상용 prompt
                const name = prompt("저장할 위치의 이름을 입력하세요:");
                if (name) sendCameraRequest(name);
            }
        });
    }

    // 4. 특징점 팝업: 확인/취소/엔터키
    if (confirmBtn) {
        confirmBtn.addEventListener('click', () => {
            const name = nameInput.value.trim();
            if (!name) {
                alert('이름을 입력해주세요!');
                return;
            }
            featurePopup.style.display = 'none';
            sendCameraRequest(name);
        });
    }

    if (cancelBtn) {
        cancelBtn.addEventListener('click', () => {
            featurePopup.style.display = 'none';
        });
    }

    if (nameInput) {
        nameInput.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') confirmBtn.click();
        });
    }

    // 5. 매핑 종료 버튼
    if (mappingBtn) {
        mappingBtn.addEventListener('click', async () => {
            if (isMappingCompleted) return;

            if (!confirm('매핑을 종료하시겠습니까? 더 이상 맵을 확장하지 않습니다.')) return;

            try {
                const response = await fetch('/mapping-complete', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ action: 'doneMapping' })
                });
                const data = await response.json();
                if (data.status === 'success') {
                    setMappingButtonToCompleted();
                    alert('매핑이 완료되었습니다.');
                    // ensure polling switches to idle interval
                    startMapUpdates();
                }
            } catch (e) {
                console.error(e);
            }
        });
    }

    // 6. 리매핑(초기화) 버튼
    if (remappingBtn) {
        remappingBtn.addEventListener('click', async () => {
            if (!confirm('⚠️ 맵을 초기화하고 다시 시작하시겠습니까?\n기존 데이터는 삭제됩니다.')) return;

            try {
                const response = await fetch('/remapping', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' }
                });
                const data = await response.json();
                if (data.status === 'success') {
                    alert('리매핑이 시작되었습니다.');
                    setMappingButtonToReady();
                    mapGrid.innerHTML = ''; // 맵 초기화
                }
            } catch (e) {
                console.error(e);
            }
        });
    }

    // 7. 매핑 완료 팝업 닫기
    if (closeMappingPopupBtn) {
        closeMappingPopupBtn.addEventListener('click', hideMappingDonePopup);
    }

    // 8. 장애물 팝업 닫기
    const closeObstaclePopupBtn = document.getElementById('close-obstacle-popup');
    if (closeObstaclePopupBtn) {
        closeObstaclePopupBtn.addEventListener('click', hideObstaclePopup);
    }

    // 9. 팝업 배경 클릭 시 닫기 (공통)
    const obstaclePopup = document.getElementById('obstacle-popup');
    window.addEventListener('click', (e) => {
        if (e.target === featurePopup) featurePopup.style.display = 'none';
        if (e.target === mappingDonePopup) hideMappingDonePopup();
        if (e.target === obstaclePopup) hideObstaclePopup();
    });
});

// ==========================================
// 6. 페이지 상태 관리 (성능 최적화)
// ==========================================

// 탭이 숨겨지면 통신 중단, 다시 열리면 재개
document.addEventListener('visibilitychange', () => {
    if (document.hidden) {
        stopMapUpdates();
    } else {
        startMapUpdates();
    }
});

// 페이지 떠날 때 정리
window.addEventListener('beforeunload', () => {
    stopMapUpdates();
});