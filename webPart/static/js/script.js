
const directionButtons = document.querySelectorAll('.direction-btn');
const mappingCompleteBtn = document.getElementById('mapping-complete');
const mapGrid = document.getElementById('map-grid');

let mapUpdateInterval = null;

const directionText = {
    'up': '⬆️ 위쪽',
    'down': '⬇️ 아래쪽',
    'left': '⬅️ 왼쪽',
    'right': '➡️ 오른쪽'
};

// 맵 표시 함수
function displayMap(mapData) {
    if (!mapData || !Array.isArray(mapData) || mapData.length === 0) {
        mapGrid.innerHTML = '<div class="map-status">맵 데이터가 없습니다.</div>';
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
            if (value === 0) {
                cell.classList.add('empty');
                cell.textContent = '0';
            } else if (value === 1) {
                cell.classList.add('path');
                cell.textContent = '1';
            } else if (value === 2) {
                cell.classList.add('feature');
                cell.textContent = '2';
            } else {
                cell.classList.add('empty');
                cell.textContent = value;
            }
            
            mapGrid.appendChild(cell);
        }
    }
}

// 맵 데이터 가져오기 함수
async function fetchMapData() {
    try {
        const response = await fetch('/get-map');
        const data = await response.json();
        
        if (data.status === 'success' && data.map) {
            displayMap(data.map);
        } else if (data.status === 'no_map') {
            mapGrid.innerHTML = `<div class="map-status">${data.message || '맵이 아직 생성되지 않았습니다.'}</div>`;
        } else if (data.status === 'error') {
            console.error('맵 데이터 가져오기 실패:', data.message);
            if (data.message.includes('Connection refused') || data.message.includes('server not running')) {
                mapGrid.innerHTML = '<div class="map-status">C++ 서버가 실행되지 않았습니다.</div>';
            } else {
                mapGrid.innerHTML = '<div class="map-status">맵 데이터 요청 중 오류 발생</div>';
            }
        } else {
            console.error('맵 데이터 가져오기 실패:', data.message);
        }
    } catch (error) {
        console.error('맵 데이터 요청 오류:', error);
        mapGrid.innerHTML = '<div class="map-status">네트워크 연결 오류</div>';
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
            } else {
                console.error('❌ 서버 오류:', data.message);
            }
        } catch (error) {
            console.error('❌ 네트워크 오류:', error);
        }
    });
});

// 매핑 완료 버튼 이벤트 리스너
mappingCompleteBtn.addEventListener('click', async function() {
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
        } else {
            console.error('❌ 서버 오류:', data.message);
        }
    } catch (error) {
        console.error('❌ 네트워크 오류:', error);
    }
});

console.log('🎮 방향 컨트롤러가 준비되었습니다!');

// 페이지 로드 시 맵 업데이트 시작
document.addEventListener('DOMContentLoaded', function() {
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
