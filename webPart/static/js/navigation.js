/**
 * 로봇 네비게이션 관련 기능
 */

// 특정 위치로 이동 (실시간 단계별 업데이트)
async function moveToLocation(locationName, progressCallback) {
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
            console.log(`✅ ${locationName}로 이동 완료!`);
            console.log(`총 ${data.completed_steps}단계 이동:`, data.steps);
            
            // 각 단계별로 콜백 실행
            if (progressCallback) {
                data.steps.forEach((step, index) => {
                    progressCallback({
                        step: step.step,
                        direction: step.direction,
                        total: data.total_steps,
                        isComplete: index === data.steps.length - 1
                    });
                });
            }
            
            return { success: true, data };
        } else if (data.status === 'failed') {
            console.error(`❌ ${locationName}로 이동 실패:`, data.error);
            return { success: false, error: data.error, data };
        } else {
            console.warn(`⚠️ 예상치 못한 상태:`, data);
            return { success: false, data };
        }

    } catch (error) {
        console.error('이동 요청 중 오류:', error);
        return { success: false, error: error.message };
    }
}

// UI에 이동 진행상황 표시
function updateNavigationUI(progress) {
    const progressText = `이동 중... ${progress.step}/${progress.total} - ${progress.direction}`;
    console.log(progressText);
    
    // 상태 텍스트 업데이트
    const statusElement = document.getElementById('navigation-status');
    if (statusElement) {
        statusElement.textContent = progressText;
        statusElement.classList.remove('completed');
        
        if (progress.isComplete) {
            statusElement.textContent = '✅ 목적지 도착!';
            statusElement.classList.add('completed');
        }
    }

    // 진행률 바 업데이트
    const progressBar = document.getElementById('navigation-progress');
    if (progressBar) {
        const percentage = (progress.step / progress.total) * 100;
        progressBar.style.width = `${percentage}%`;
    }
}

// 여러 위치로 순차 이동
async function moveToMultipleLocations(locations, progressCallback) {
    const results = [];
    
    for (const location of locations) {
        console.log(`\n🚀 ${location}으로 이동 시작...`);
        
        const result = await moveToLocation(location, progressCallback || updateNavigationUI);
        
        results.push({ location, ...result });
        
        if (!result.success) {
            console.error(`${location}에서 중단됨`);
            break;
        }
        
        // 다음 이동 전 잠시 대기
        await new Promise(resolve => setTimeout(resolve, 1000));
    }
    
    return results;
}

// Feature 목록 로드 및 버튼 생성
async function loadNavigationLocations() {
    const infoElement = document.getElementById('quick-nav-info');
    const gridElement = document.getElementById('quick-nav-grid');
    
    if (!gridElement) return;
    
    try {
        const response = await fetch('/api/features');
        const data = await response.json();
        
        if (data.status === 'success' && data.features && data.features.length > 0) {
            infoElement.textContent = `총 ${data.features.length}개의 위치`;
            gridElement.innerHTML = '';
            
            data.features.forEach(featureName => {
                const btn = document.createElement('button');
                btn.className = 'nav-location-btn';
                btn.dataset.location = featureName;
                btn.innerHTML = `
                    <span class="nav-location-icon">📍</span>
                    <span class="nav-location-name">${featureName}</span>
                `;
                
                btn.addEventListener('click', async (e) => {
                    const location = e.currentTarget.dataset.location;
                    const allButtons = document.querySelectorAll('.nav-location-btn');
                    
                    // 모든 버튼 비활성화
                    allButtons.forEach(b => b.disabled = true);
                    e.currentTarget.innerHTML = `
                        <span class="nav-location-icon">⏳</span>
                        <span class="nav-location-name">이동중...</span>
                    `;
                    
                    // 이동 시작
                    const result = await moveToLocation(location, updateNavigationUI);
                    
                    // 버튼 상태 복원
                    if (result.success) {
                        e.currentTarget.innerHTML = `
                            <span class="nav-location-icon">✅</span>
                            <span class="nav-location-name">${location}</span>
                        `;
                        
                        // Searching 모드 전환 확인
                        if (confirm(`✅ ${location}으로 이동 완료!\n\nSearching 모드로 전환하시겠습니까?`)) {
                            try {
                                const navDoneResponse = await fetch('/navigatedone', {
                                    method: 'POST',
                                    headers: {
                                        'Content-Type': 'application/json'
                                    }
                                });
                                
                                const navDoneData = await navDoneResponse.json();
                                
                                if (navDoneData.status === 'success') {
                                    const statusElement = document.getElementById('navigation-status');
                                    if (statusElement) {
                                        statusElement.textContent = `🔍 ${navDoneData.message}`;
                                        statusElement.classList.add('completed');
                                    }
                                } else {
                                    alert(`⚠️ Searching 모드 전환 실패: ${navDoneData.message}`);
                                }
                            } catch (error) {
                                console.error('Searching 모드 전환 오류:', error);
                                alert('❌ Searching 모드 전환 중 오류가 발생했습니다.');
                            }
                        }
                    } else {
                        e.currentTarget.innerHTML = `
                            <span class="nav-location-icon">❌</span>
                            <span class="nav-location-name">${location}</span>
                        `;
                    }
                    
                    // 2초 후 원래 상태로 복원
                    setTimeout(() => {
                        e.currentTarget.innerHTML = `
                            <span class="nav-location-icon">📍</span>
                            <span class="nav-location-name">${location}</span>
                        `;
                        allButtons.forEach(b => b.disabled = false);
                    }, 2000);
                });
                
                gridElement.appendChild(btn);
            });
        } else if (data.status === 'no_features' || data.features.length === 0) {
            infoElement.textContent = '등록된 위치가 없습니다. 카메라 버튼으로 위치를 등록하세요.';
            gridElement.innerHTML = '<p style="color: #999; grid-column: 1/-1;">특징점을 먼저 촬영해주세요 📸</p>';
        } else {
            infoElement.textContent = '위치 정보를 불러올 수 없습니다.';
        }
    } catch (error) {
        console.error('Feature 목록 로드 오류:', error);
        infoElement.textContent = '위치 정보를 불러오는 중 오류가 발생했습니다.';
    }
}

// 페이지 로드 시 초기화
document.addEventListener('DOMContentLoaded', () => {
    // Feature 목록 로드
    loadNavigationLocations();
    
    // 10초마다 목록 새로고침 (새로운 특징점이 추가될 수 있음)
    setInterval(loadNavigationLocations, 10000);
});
