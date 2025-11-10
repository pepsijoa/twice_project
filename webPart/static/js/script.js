
const directionButtons = document.querySelectorAll('.direction-btn');
const mappingBtn = document.getElementById('mapping-btn');
const remappingBtn = document.getElementById('remapping-btn');
const cameraBtn = document.getElementById('camera-btn');
const mapGrid = document.getElementById('map-grid');

let mapUpdateInterval = null;
let isMappingCompleted = false; // 매핑 완료 상태 추적

const directionText = {
    'up': '⬆️ 위쪽',
    'down': '⬇️ 아래쪽',
    'left': '⬅️ 왼쪽',
    'right': '➡️ 오른쪽'
};

// 맵 표시 함수 (개선된 버전)
function displayMap(mapData) {
    if (!mapData || !Array.isArray(mapData) || mapData.length === 0) {
        mapGrid.innerHTML = '<div class="map-status">🗺️ 맵 데이터가 없습니다.</div>';
        return;
    }
    
    const height = mapData.length;
    const width = mapData[0].length;
    
    // CSS Grid 설정
    mapGrid.style.gridTemplateColumns = `repeat(${width}, 1fr)`;
    mapGrid.style.gridTemplateRows = `repeat(${height}, 1fr)`;
    
    // 맵 셀 생성
    mapGrid.innerHTML = '';
    
    // 위에서 아래로 표시 (배열의 마지막 행부터 첫 번째 행 순서)
    for (let y = height - 1; y >= 0; y--) {
        for (let x = 0; x < width; x++) {
            const cell = document.createElement('div');
            cell.className = 'map-cell';
            
            const value = mapData[y][x];
            
            // 값에 따른 스타일 및 아이콘 설정
            if (value === 0) {
                cell.classList.add('empty');
                cell.textContent = '⬜';  // 빈 공간 아이콘
                cell.title = `빈 공간 (${x}, ${height-1-y})`;
            } else if (value === 1) {
                cell.classList.add('path');
                cell.textContent = '🟢';  // 경로 아이콘
                cell.title = `이동 경로 (${x}, ${height-1-y})`;
            } else if (value === 2) {
                cell.classList.add('feature');
                cell.textContent = '🔶';  // 특징점 아이콘
                cell.title = `특징점/장애물 (${x}, ${height-1-y})`;
            } else if (value === 3) {
                cell.classList.add('current-position');
                cell.textContent = '🤖';  // 로봇 현재 위치
                cell.title = `로봇 현재 위치 (${x}, ${height-1-y})`;
            } else {
                cell.classList.add('empty');
                cell.textContent = value;
                cell.title = `알 수 없는 값: ${value} (${x}, ${height-1-y})`;
            }
            
            // 호버 효과를 위한 좌표 정보 추가
            cell.setAttribute('data-x', x);
            cell.setAttribute('data-y', height-1-y);
            
            mapGrid.appendChild(cell);
        }
    }
    
    // 맵 크기 정보 표시
    const mapInfo = document.getElementById('map-info');
    if (mapInfo) {
        mapInfo.textContent = `맵 크기: ${width} × ${height}`;
    }
}

// 맵 데이터 가져오기 함수 (개선된 버전)
async function fetchMapData() {
    try {
        const response = await fetch('/get-map');
        const data = await response.json();
        
        if (data.status === 'success' && data.map) {
            displayMap(data.map);
        } else if (data.status === 'no_map') {
            mapGrid.innerHTML = `<div class="map-status">🔄 ${data.message || '맵이 아직 생성되지 않았습니다.'}</div>`;
        } else if (data.status === 'error') {
            console.error('맵 데이터 가져오기 실패:', data.message);
            if (data.message.includes('Connection refused') || data.message.includes('server not running')) {
                mapGrid.innerHTML = '<div class="map-status">⚠️ C++ 서버가 실행되지 않았습니다.</div>';
            } else {
                mapGrid.innerHTML = '<div class="map-status">❌ 맵 데이터 요청 중 오류 발생</div>';
            }
        } else {
            console.error('맵 데이터 가져오기 실패:', data.message);
            mapGrid.innerHTML = '<div class="map-status">❓ 알 수 없는 오류가 발생했습니다.</div>';
        }
    } catch (error) {
        console.error('맵 데이터 요청 오류:', error);
        mapGrid.innerHTML = '<div class="map-status">🌐 네트워크 연결 오류</div>';
    }
}

// 맵 업데이트 시작
function startMapUpdates() {
    // 즉시 한 번 실행
    fetchMapData();
    
    mapUpdateInterval = setInterval(fetchMapData, 1000);
}

// 맵 업데이트 중지
function stopMapUpdates() {
    if (mapUpdateInterval) {
        clearInterval(mapUpdateInterval);
        mapUpdateInterval = null;
    }
}

// 매핑 버튼 상태 업데이트 함수들
function setMappingButtonToReady() {
    if (mappingBtn) {
        mappingBtn.setAttribute('data-state', 'ready');
        mappingBtn.querySelector('.mapping-text').textContent = '매핑';
        mappingBtn.disabled = false;
        isMappingCompleted = false;
    }
}

function setMappingButtonToCompleted() {
    if (mappingBtn) {
        mappingBtn.setAttribute('data-state', 'completed');
        mappingBtn.querySelector('.mapping-text').textContent = '매핑 완료';
        mappingBtn.disabled = true;
        isMappingCompleted = true;
    }
}

// 각 버튼에 클릭 이벤트 리스너 추가
directionButtons.forEach(button => {
    button.addEventListener('click', async function() {
        const direction = this.getAttribute('data-direction');
        this.classList.add('clicked');
        
        setTimeout(() => {
            this.classList.remove('clicked');
        }, 300);
        
        try {
            const response = await fetch('/control', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ direction: direction })
            });
            
            const data = await response.json();
            
            if (data.status === 'success') {
                console.log(`✅ 서버 응답 성공: ${data.direction}`);
            } else if (data.status === 'mapping_done') {
                console.log(`🗺️ 매핑 완료: ${data.message}`);
                showMappingDonePopup();
            } else {
                console.error('❌ 서버 오류:', data.message);
            }
        } catch (error) {
            console.error('❌ 네트워크 오류:', error);
        }
    });
});

// 매핑 버튼 이벤트 리스너
mappingBtn.addEventListener('click', async function() {
    // 매핑이 완료된 상태면 클릭 무시
    if (isMappingCompleted) {
        return;
    }
    
    this.classList.add('clicked');
    
    setTimeout(() => {
        this.classList.remove('clicked');
    }, 300);
    
    try {
        const response = await fetch('/mapping-complete', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ action: 'doneMapping' })
        });
        
        const data = await response.json();
        
        if (data.status === 'success') {
            console.log('✅ 매핑 완료 신호 전송 성공');
            // 매핑 버튼을 완료 상태로 변경
            setMappingButtonToCompleted();
        } else {
            console.error('❌ 서버 오류:', data.message);
        }
    } catch (error) {
        console.error('❌ 네트워크 오류:', error);
    }
});

// 리매핑 버튼 이벤트 리스너
remappingBtn.addEventListener('click', async function() {
    this.classList.add('clicked');
    
    setTimeout(() => {
        this.classList.remove('clicked');
    }, 300);
    
    // 확인 팝업 표시
    if (!confirm('🔄 새로운 매핑을 시작하시겠습니까?\n\n현재 매핑 데이터가 모두 삭제됩니다.')) {
        return;
    }
    
    try {
        const response = await fetch('/remapping', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            }
        });
        
        const data = await response.json();
        
        if (data.status === 'success') {
            console.log('✅ 리매핑 요청 성공:', data.message);
            
            // 성공 알림
            alert(`🎉 ${data.message}`);
            
            // 맵 그리드 초기화
            mapGrid.innerHTML = '<div class="map-status">🔄 새로운 매핑이 시작되었습니다.</div>';
            
            // 매핑 버튼을 다시 준비 상태로 되돌림
            setMappingButtonToReady();
            
        } else {
            console.error('❌ 서버 오류:', data.message);
            alert(`❌ 오류: ${data.message}`);
        }
    } catch (error) {
        console.error('❌ 네트워크 오류:', error);
        alert('❌ 네트워크 연결 오류가 발생했습니다.');
    }
});

// 카메라 버튼 이벤트 리스너
cameraBtn.addEventListener('click', async function() {
    this.classList.add('clicked');
    
    setTimeout(() => {
        this.classList.remove('clicked');
    }, 300);
    
    // 버튼 비활성화 (중복 클릭 방지)
    this.disabled = true;
    const originalText = this.querySelector('.camera-text').textContent;
    this.querySelector('.camera-text').textContent = '촬영 중...';
    
    try {
        const response = await fetch('/camera', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            }
        });
        
        const data = await response.json();
        
        if (data.status === 'success') {
            console.log('✅ 카메라 촬영 성공:', data.message);
            
            // 성공 알림
            alert(`📸 ${data.message}`);
            
        } else if (data.status === 'failed') {
            console.error('❌ 카메라 촬영 실패:', data.message);
            alert(`❌ ${data.message}`);
            
        } else {
            console.error('❌ 서버 오류:', data.message);
            alert(`❌ 오류: ${data.message}`);
        }
    } catch (error) {
        console.error('❌ 네트워크 오류:', error);
        alert('❌ 네트워크 연결 오류가 발생했습니다.');
    } finally {
        // 버튼 복구
        this.disabled = false;
        this.querySelector('.camera-text').textContent = originalText;
    }
});

console.log('🎮 방향 컨트롤러가 준비되었습니다!');

// 페이지 로드 시 맵 업데이트 시작
document.addEventListener('DOMContentLoaded', function() {
    // 매핑 버튼을 기본 상태(준비)로 설정
    setMappingButtonToReady();
    
    startMapUpdates();
});

// 페이지 언로드 시 맵 업데이트 중지
window.addEventListener('beforeunload', function() {
    stopMapUpdates();
});

// 페이지가 숨겨질 때 업데이트 중지, 다시 보일 때 시작
document.addEventListener('visibilitychange', function() {
    if (document.hidden) {
        stopMapUpdates();
    } else {
        startMapUpdates();
    }
});

// 매핑 완료 팝업 관련 함수들
function showMappingDonePopup() {
    const popup = document.getElementById('mapping-done-popup');
    if (popup) {
        popup.style.display = 'flex';
        document.body.style.overflow = 'hidden'; // 배경 스크롤 방지
        
        // 자동으로 매핑 버튼을 완료 상태로 변경
        setMappingButtonToCompleted();
    }
}

function hideMappingDonePopup() {
    const popup = document.getElementById('mapping-done-popup');
    if (popup) {
        popup.style.display = 'none';
        document.body.style.overflow = 'auto'; // 배경 스크롤 복원
    }
}

// 팝업 닫기 버튼 이벤트 리스너
document.addEventListener('DOMContentLoaded', function() {
    const closeButton = document.getElementById('close-mapping-popup');
    const popup = document.getElementById('mapping-done-popup');
    
    // 닫기 버튼 클릭
    if (closeButton) {
        closeButton.addEventListener('click', hideMappingDonePopup);
    }
    
    // 팝업 배경 클릭으로도 닫기
    if (popup) {
        popup.addEventListener('click', function(e) {
            if (e.target === popup) {
                hideMappingDonePopup();
            }
        });
    }
    
    // ESC 키로 팝업 닫기
    document.addEventListener('keydown', function(e) {
        if (e.key === 'Escape') {
            hideMappingDonePopup();
        }
    });
});
