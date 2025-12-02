// 재고 관리 JavaScript

// 재고 데이터 로드
async function loadInventory() {
    try {
        const response = await fetch('/api/inventory');
        const data = await response.json();
        
        if (data.status === 'success' && data.items) {
            displayInventory(data.items);
        } else {
            showEmptyState();
        }
    } catch (error) {
        console.error('재고 데이터 로드 실패:', error);
        document.getElementById('inventoryGrid').innerHTML = 
            '<div style="text-align: center; padding: 40px; color: white;"><h3>❌ 재고 데이터를 불러올 수 없습니다.</h3><p>데이터베이스 연결을 확인해주세요.</p></div>';
    }
}

// 재고 카드 표시
function displayInventory(items) {
    const grid = document.getElementById('inventoryGrid');
    const emptyState = document.getElementById('emptyState');
    
    if (!items || items.length === 0) {
        grid.style.display = 'none';
        emptyState.style.display = 'block';
        return;
    }
    
    grid.style.display = 'grid';
    emptyState.style.display = 'none';
    
    grid.innerHTML = items.map(item => createInventoryCard(item)).join('');
}

// 재고 카드 HTML 생성
function createInventoryCard(item) {
    const quantityClass = item.quantity >= 10 ? 'high' : (item.quantity >= 5 ? 'medium' : 'low');
    
    return `
        <div class="inventory-card" data-id="${item.id}">
            <div class="item-name">${escapeHtml(item.name)}</div>
            <div class="item-info">
                <span class="item-label">수량:</span>
                <span class="quantity ${quantityClass}" id="quantity-${item.id}">${item.quantity}개</span>
                <div class="quantity-control">
                    <button class="quantity-btn" onclick="changeQuantity(${item.id}, -1)" title="수량 감소">−</button>
                    <button class="quantity-btn" onclick="changeQuantity(${item.id}, 1)" title="수량 증가">+</button>
                </div>
            </div>
            <div class="item-info">
                <span class="item-label">위치:</span> ${escapeHtml(item.location)}
            </div>
            <div class="item-info">
                <span class="item-label">최종 업데이트:</span> ${item.updated_at || '미상'}
            </div>
            <div class="item-actions">
                <button class="action-btn edit-btn" onclick="editItem(${item.id})">✏️ 수정</button>
                <button class="action-btn map-btn" onclick="showMap('${escapeHtml(item.name)}', '${escapeHtml(item.location)}')">🗺️ 위치</button>
                <button class="action-btn delete-btn" onclick="deleteItem(${item.id})">🗑️ 삭제</button>
            </div>
        </div>
    `;
}

// HTML 이스케이프 함수 (XSS 방지)
function escapeHtml(text) {
    const map = {
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;',
        '"': '&quot;',
        "'": '&#039;'
    };
    return text.replace(/[&<>"']/g, m => map[m]);
}

// 빈 상태 표시
function showEmptyState() {
    document.getElementById('inventoryGrid').style.display = 'none';
    document.getElementById('emptyState').style.display = 'block';
}

// 재고 추가 함수
async function addItem() {
    console.log('✅ addItem 함수 호출됨');
    
    // 1. 제품 이름 입력
    const name = prompt('제품 이름을 입력하세요:');
    if (!name) {
        console.log('❌ 이름 입력 취소됨');
        return;
    }
    console.log('입력된 이름:', name);
    
    // 2. 수량 입력
    const quantity = parseInt(prompt('수량을 입력하세요:', '0'));
    if (isNaN(quantity)) return;
    
    // 3. Feature 목록 가져오기
    let features = [];
    let location = '';
    
    try {
        const featureResponse = await fetch('/api/features');
        const featureData = await featureResponse.json();
        
        if (featureData.status === 'success' && featureData.features && featureData.features.length > 0) {
            features = featureData.features;
            
            // 4. Feature 위치 선택 (번호 입력 방식)
            let locationMessage = '저장된 위치 목록:\n\n';
            features.forEach((feature, index) => {
                locationMessage += `${index + 1}. ${feature}\n`;
            });
            locationMessage += '\n위치 번호를 입력하세요 (직접 입력: 0):';
            
            const locationIndex = parseInt(prompt(locationMessage));
            
            if (locationIndex === 0) {
                // 직접 입력
                location = prompt('위치를 직접 입력하세요:');
                if (!location) return;
            } else if (!isNaN(locationIndex) && locationIndex >= 1 && locationIndex <= features.length) {
                location = features[locationIndex - 1];
            } else {
                alert('❌ 올바른 위치 번호를 입력해주세요.');
                return;
            }
        } else {
            // Feature가 없으면 직접 입력
            alert('⚠️ 등록된 특징점이 없습니다. 위치를 직접 입력해주세요.');
            location = prompt('위치를 입력하세요:');
            if (!location) return;
        }
    } catch (error) {
        console.error('Feature 목록 로드 실패:', error);
        alert('⚠️ 특징점 목록을 불러올 수 없습니다. 위치를 직접 입력해주세요.');
        location = prompt('위치를 입력하세요:');
        if (!location) return;
    }
    
    // 5. 서버에 재고 추가 요청
    try {
        const response = await fetch('/api/inventory', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ name, quantity, location })
        });
        
        const data = await response.json();
        
        if (data.status === 'success') {
            alert(`✅ 재고가 추가되었습니다.\n위치: ${location}`);
            loadInventory();
        } else {
            alert('❌ 재고 추가 실패: ' + data.message);
        }
    } catch (error) {
        console.error('재고 추가 오류:', error);
        alert('❌ 재고 추가 중 오류가 발생했습니다.');
    }
}

// 수량 변경 함수
async function changeQuantity(id, change) {
    try {
        // 현재 재고 정보 가져오기
        const response = await fetch('/api/inventory');
        const data = await response.json();
        const item = data.items.find(i => i.id === id);
        
        if (!item) {
            alert('재고를 찾을 수 없습니다.');
            return;
        }
        
        const newQuantity = item.quantity + change;
        
        // 수량이 0 미만이 되지 않도록 체크
        if (newQuantity < 0) {
            alert('❌ 수량은 0 미만이 될 수 없습니다.');
            return;
        }
        
        // 서버에 수량 업데이트 요청
        const updateResponse = await fetch(`/api/inventory/${id}`, {
            method: 'PUT',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ 
                name: item.name, 
                quantity: newQuantity, 
                location: item.location 
            })
        });
        
        const updateData = await updateResponse.json();
        
        if (updateData.status === 'success') {
            // UI 즉시 업데이트
            const quantityElement = document.getElementById(`quantity-${id}`);
            if (quantityElement) {
                quantityElement.textContent = `${newQuantity}개`;
                
                // 수량에 따라 색상 클래스 변경
                quantityElement.className = 'quantity';
                if (newQuantity >= 10) {
                    quantityElement.classList.add('high');
                } else if (newQuantity >= 5) {
                    quantityElement.classList.add('medium');
                } else {
                    quantityElement.classList.add('low');
                }
            }
        } else {
            alert('❌ 수량 변경 실패: ' + updateData.message);
        }
    } catch (error) {
        console.error('수량 변경 오류:', error);
        alert('❌ 수량 변경 중 오류가 발생했습니다.');
    }
}

// 재고 수정 함수
async function editItem(id) {
    try {
        // 1. 기존 재고 정보 가져오기
        const response = await fetch('/api/inventory');
        const data = await response.json();
        const item = data.items.find(i => i.id === id);
        
        if (!item) {
            alert('재고를 찾을 수 없습니다.');
            return;
        }
        
        // 2. 제품 이름 수정
        const name = prompt('제품 이름:', item.name);
        if (name === null) return;
        
        // 3. 수량 수정
        const quantity = parseInt(prompt('수량:', item.quantity));
        if (isNaN(quantity)) return;
        
        // 4. 위치 변경 여부 확인
        const changeLocation = confirm(`현재 위치: ${item.location}\n\n위치를 변경하시겠습니까?`);
        let location = item.location;
        
        if (changeLocation) {
            // Feature 목록 가져오기
            try {
                const featureResponse = await fetch('/api/features');
                const featureData = await featureResponse.json();
                
                if (featureData.status === 'success' && featureData.features && featureData.features.length > 0) {
                    const features = featureData.features;
                    
                    let locationMessage = '저장된 위치 목록:\n\n';
                    features.forEach((feature, index) => {
                        locationMessage += `${index + 1}. ${feature}\n`;
                    });
                    locationMessage += `\n현재 위치: ${item.location}\n\n새로운 위치 번호를 입력하세요 (취소: 0):`;
                    
                    const locationIndex = parseInt(prompt(locationMessage));
                    
                    if (locationIndex === 0) {
                        // 취소
                        location = item.location;
                    } else if (!isNaN(locationIndex) && locationIndex >= 1 && locationIndex <= features.length) {
                        location = features[locationIndex - 1];
                    } else {
                        alert('❌ 올바른 위치 번호를 입력해주세요.');
                        return;
                    }
                } else {
                    alert('⚠️ 등록된 특징점이 없습니다. 기존 위치를 유지합니다.');
                }
            } catch (error) {
                console.error('Feature 목록 로드 실패:', error);
                alert('❌ 특징점 목록을 불러올 수 없습니다. 기존 위치를 유지합니다.');
            }
        }
        
        // 5. 서버에 수정 요청
        const updateResponse = await fetch(`/api/inventory/${id}`, {
            method: 'PUT',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ name, quantity, location })
        });
        
        const updateData = await updateResponse.json();
        
        if (updateData.status === 'success') {
            alert('✅ 재고가 수정되었습니다.');
            loadInventory();
        } else {
            alert('❌ 재고 수정 실패: ' + updateData.message);
        }
    } catch (error) {
        console.error('재고 수정 오류:', error);
        alert('❌ 재고 수정 중 오류가 발생했습니다.');
    }
}

// 재고 삭제 함수
async function deleteItem(id) {
    if (!confirm('정말 삭제하시겠습니까?')) {
        return;
    }
    
    try {
        const response = await fetch(`/api/inventory/${id}`, {
            method: 'DELETE'
        });
        
        const data = await response.json();
        
        if (data.status === 'success') {
            alert('✅ 재고가 삭제되었습니다.');
            loadInventory();
        } else {
            alert('❌ 재고 삭제 실패: ' + data.message);
        }
    } catch (error) {
        console.error('재고 삭제 오류:', error);
        alert('❌ 재고 삭제 중 오류가 발생했습니다.');
    }
}

// 검색 기능 초기화
function initializeSearch() {
    document.getElementById('searchInput').addEventListener('input', async function(e) {
        const searchTerm = e.target.value.toLowerCase();
        
        if (searchTerm === '') {
            loadInventory();
            return;
        }
        
        try {
            const response = await fetch(`/api/inventory/search?q=${encodeURIComponent(searchTerm)}`);
            const data = await response.json();
            
            if (data.status === 'success') {
                displayInventory(data.items);
            }
        } catch (error) {
            console.error('검색 실패:', error);
        }
    });
}

// 페이지 로드 시 초기화
document.addEventListener('DOMContentLoaded', function() {
    console.log('📦 재고 관리 페이지 로드됨');
    
    // Service Worker 등록
    if ('serviceWorker' in navigator) {
        navigator.serviceWorker.register('/static/sw.js')
            .then(reg => console.log('Service Worker registered'))
            .catch(err => console.log('Service Worker registration failed'));
    }
    
    // 재고 로드
    loadInventory();
    
    // 검색 기능 초기화
    initializeSearch();
    
    // 재고 추가 버튼 이벤트 리스너 등록
    const addItemBtn = document.getElementById('addItemBtn');
    if (addItemBtn) {
        addItemBtn.addEventListener('click', function() {
            console.log('✅ 버튼 클릭됨');
            addItem();
        });
    }
});
