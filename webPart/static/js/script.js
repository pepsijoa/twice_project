
const directionButtons = document.querySelectorAll('.direction-btn');
const mappingCompleteBtn = document.getElementById('mapping-complete');
const statusText = document.getElementById('status-text');

const directionText = {
    'up': '⬆️ 위쪽',
    'down': '⬇️ 아래쪽',
    'left': '⬅️ 왼쪽',
    'right': '➡️ 오른쪽'
};

// 각 버튼에 클릭 이벤트 리스너 추가
directionButtons.forEach(button => {
    button.addEventListener('click', async function() {
        const direction = this.getAttribute('data-direction');
        this.classList.add('clicked');
        
        setTimeout(() => {
            this.classList.remove('clicked');
        }, 300);
        
        statusText.textContent = `${directionText[direction]} 버튼 클릭됨!`;
        statusText.style.color = '#f5576c';
        
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
                statusText.textContent = '오류가 발생했습니다!';
                statusText.style.color = '#ff0000';
            }
        } catch (error) {
            console.error('❌ 네트워크 오류:', error);
            statusText.textContent = '서버 연결 실패!';
            statusText.style.color = '#ff0000';
        }
    });
});

// 매핑 완료 버튼 이벤트 리스너
mappingCompleteBtn.addEventListener('click', async function() {
    this.classList.add('clicked');
    
    setTimeout(() => {
        this.classList.remove('clicked');
    }, 300);
    
    statusText.textContent = '🗺️ 매핑을 완료합니다!';
    statusText.style.color = '#4CAF50';
    
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
            statusText.textContent = '✅ 매핑 완료!';
            statusText.style.color = '#4CAF50';
        } else {
            console.error('❌ 서버 오류:', data.message);
            statusText.textContent = '오류가 발생했습니다!';
            statusText.style.color = '#ff0000';
        }
    } catch (error) {
        console.error('❌ 네트워크 오류:', error);
        statusText.textContent = '서버 연결 실패!';
        statusText.style.color = '#ff0000';
    }
});

console.log('🎮 방향 컨트롤러가 준비되었습니다!');
