# 🚀 빠른 시작 가이드 (Quick Start)

## 1분 안에 시작하기

### 방법 1: 자동 시작 스크립트 사용 (권장)

```bash
./start.sh
```

이 스크립트는 자동으로:
- ✅ 가상환경 활성화
- ✅ Flask 설치 확인
- ✅ SSL 인증서 생성 (선택)
- ✅ 서버 시작
- ✅ 접속 주소 표시

---

### 방법 2: 수동 실행

```bash
# 1. 가상환경 활성화
source venv/bin/activate

# 2. Flask 설치 (처음 한 번만)
pip install flask

# 3. HTTPS 설정 (선택사항)
python certification/generate_cert.py

# 4. 서버 시작
python app.py
```

---

## 📱 접속하기

### PC에서
```
https://localhost:5000
```

### 모바일에서
1. **같은 WiFi 연결** 확인
2. **IP 주소 확인**:
   ```bash
   hostname -I
   ```
3. **브라우저에서 접속**:
   ```
   https://192.168.x.x:5000
   ```

---

## 🔒 HTTPS 인증서 경고

자체 서명 인증서이므로 경고가 표시됩니다. 안전하게 진행하세요:

### Chrome/Edge
1. "고급" 클릭
2. "localhost(으)로 이동(안전하지 않음)" 클릭

### Safari
1. "세부사항 보기" 클릭
2. "웹 사이트 방문" 클릭

### 모바일
1. "고급" 또는 "자세히" 클릭
2. "이동" 또는 "계속" 클릭

---

## 🎮 사용 방법

1. **버튼 클릭**: 화면의 ⬆️ ⬇️ ⬅️ ➡️ 버튼 클릭
2. **키보드**: 방향키로 제어 (PC)
3. **터미널**: 서버 터미널에서 출력 확인

---

## 📲 앱으로 설치 (PWA)

### iOS
1. Safari에서 접속
2. 공유 버튼 ⬆️
3. "홈 화면에 추가"

### Android
1. Chrome에서 접속
2. 메뉴 ⋮
3. "홈 화면에 추가"

---

## ❓ 문제 해결

### 포트 사용 중
```bash
sudo lsof -i :5000
sudo kill -9 <PID>
```

### OpenSSL 없음
```bash
sudo apt-get install openssl
```

### Flask 없음
```bash
pip install flask
```

---

## 📚 더 자세한 내용

전체 문서는 [README.md](README.md)를 참고하세요.
