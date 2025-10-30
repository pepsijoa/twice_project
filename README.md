# 🎮 IoT 컨트롤러 (IoT Controller)

Flask 기반의 IoT 기기 제어 웹 애플리케이션입니다.
PWA(Progressive Web App)로 제작되어 모바일 기기에서도 앱처럼 사용할 수 있습니다.

## 📚 문서 가이드

- 📖 [빠른 시작 가이드](QUICKSTART.md) - 1분 안에 시작하기
- 🔒 [HTTPS 경고 제거 방법](certification/REMOVE_WARNING.md) - 인증서 경고 해결
- 📱 [모바일 인증서 설치](certification/MOBILE_CERT_GUIDE.md) - iOS/Android 상세 가이드

## ✨ 주요 기능

- 🏠 메인 대시보드 (다중 기능 메뉴)
- �️ 방향 제어 컨트롤러 (상하좌우)
- 🔒 HTTP/HTTPS 지원 (자동 전환)
- 📱 모바일 최적화 (PWA 지원)
- ⌨️ 키보드 방향키 지원
- 🎨 반응형 디자인
- 💾 오프라인 지원 (Service Worker)
- 📥 인증서 다운로드 기능

## 🚀 설치 및 실행

### 1. 가상환경 생성 및 활성화
```bash
cd twiceProjeect
python3 -m venv venv
source venv/bin/activate
```

### 2. Flask 설치
```bash
pip install flask
```

### 3. HTTPS 설정 (선택사항, 권장)
PWA 기능을 완전히 사용하려면 HTTPS가 필요합니다.
```bash
# SSL 인증서 생성
python certification/generate_cert.py

# OpenSSL이 없다면 설치
sudo apt-get install openssl
```

### 4. 아이콘 생성 (선택사항)
아이콘 이미지가 없다면 브라우저에서 생성:
```bash
# 브라우저에서 아래 URL 접속하여 자동 다운로드
http://localhost:5000/static/generate_icons.html
```
다운로드된 `icon-192.png`와 `icon-512.png`를 `static/images/` 폴더에 저장

### 5. 서버 실행
```bash
python app.py
```

> **💡 팁**: 
> - 인증서가 있으면 자동으로 HTTPS로 실행됩니다.
> - 인증서가 없으면 HTTP로 실행됩니다.

## 🌐 접속 방법

### PC에서 접속
```
# HTTP (기본)
http://localhost:5000

# HTTPS (인증서 생성 후)
https://localhost:5000
```

### 모바일에서 접속
1. **같은 네트워크에 연결** (PC와 동일한 WiFi)
2. **라즈베리파이 IP 확인**
   ```bash
   hostname -I
   ```
3. **모바일 브라우저에서 접속**
   ```
   # HTTP
   http://<라즈베리파이IP>:5000
   
   # HTTPS (권장)
   https://<라즈베리파이IP>:5000
   ```
   예: `https://192.168.0.10:5000`

> **⚠️ HTTPS 경고 해결 방법**:
> - 자체 서명 인증서이므로 "안전하지 않음" 경고가 표시됩니다.
> - **PC**: "고급" → "계속 진행" 클릭
> - **모바일**: "고급" → "안전하지 않음" → "이동" 클릭
> - 이는 정상이며, 로컬 네트워크에서는 안전합니다.

### 📲 홈 화면에 추가하기

#### iOS (Safari)
1. 사이트 접속
2. 공유 버튼 (⬆️) 클릭
3. "홈 화면에 추가" 선택
4. 추가 완료!

#### Android (Chrome)
1. 사이트 접속
2. 메뉴 버튼 (⋮) 클릭
3. "홈 화면에 추가" 또는 "앱 설치" 선택
4. 추가 완료!

## 📁 프로젝트 구조

```
twiceProjeect/
├── app.py                      # Flask 서버 (HTTP/HTTPS 지원)
├── start.sh                    # 서버 시작 스크립트
├── certification/              # 인증서 관련 파일 모음
│   ├── generate_cert.py        # SSL 인증서 생성 스크립트
│   ├── generate_cert_advanced.py # 고급 인증서 생성
│   ├── trust_cert.sh           # 인증서 신뢰 추가 스크립트
│   ├── certs/                  # SSL 인증서 (자동 생성)
│   │   ├── cert.pem            # SSL 인증서
│   │   └── key.pem             # SSL 개인키
│   ├── REMOVE_WARNING.md       # 경고 제거 가이드
│   └── MOBILE_CERT_GUIDE.md    # 모바일 인증서 설치 가이드
├── templates/
│   ├── main.html               # 메인 대시보드
│   └── controller.html         # 방향 컨트롤러 페이지
├── static/
│   ├── css/
│   │   └── style.css           # 스타일시트
│   ├── js/
│   │   └── script.js           # JavaScript
│   ├── images/
│   │   ├── icon-192.png        # 앱 아이콘 (192x192)
│   │   └── icon-512.png        # 앱 아이콘 (512x512)
│   ├── manifest.json           # PWA 매니페스트
│   └── sw.js                   # Service Worker
└── venv/                       # Python 가상환경
```

## 🎯 사용 방법

### 메인 페이지 (`/`)
- 🏠 IoT 컨트롤러 대시보드
- 여러 기능 중 선택 가능
- 방향 컨트롤러, 모니터링, 설정 등 (일부 준비 중)

### 방향 컨트롤러 (`/controller`)
1. **버튼 클릭**: 상하좌우 버튼을 클릭하면 서버에서 방향 정보를 받아 처리
2. **키보드 제어**: PC에서 방향키(←↑→↓)로도 제어 가능
3. **터미널 출력**: 버튼 클릭 시 서버 터미널에 이모지와 함께 메시지 출력
4. **홈 버튼**: 언제든지 메인 페이지로 돌아갈 수 있음

## 🔒 HTTPS 설정 가이드

### 왜 HTTPS가 필요한가요?
- 📱 **PWA 기능**: Service Worker, 오프라인 모드 등은 HTTPS에서만 작동
- 🔐 **보안**: 데이터 암호화 및 안전한 통신
- 📲 **홈 화면 추가**: iOS와 Android에서 PWA 설치를 위해 필요

### HTTPS 설정 방법

#### 1단계: SSL 인증서 생성
```bash
python certification/generate_cert.py
```
python generate_cert.py
```

#### 2단계: 서버 실행
```bash
python app.py
```
> 인증서가 있으면 자동으로 HTTPS로 실행됩니다.

#### 3단계: 브라우저에서 접속
```
https://localhost:5000
또는
https://<라즈베리파이IP>:5000
```

#### 4단계: 인증서 경고 수락
- **Chrome/Edge**: "고급" → "localhost(으)로 이동(안전하지 않음)"
- **Safari**: "세부사항 보기" → "웹 사이트 방문"
- **모바일**: "고급" → "이동" 또는 "계속"

### 문제 해결

**OpenSSL이 없다면:**
```bash
sudo apt-get update
sudo apt-get install openssl
```

**포트가 사용 중이라면:**
```bash
# 5000번 포트를 사용하는 프로세스 확인
sudo lsof -i :5000

# 프로세스 종료
sudo kill -9 <PID>
```

## �🛠️ 기술 스택

- **Backend**: Python Flask (HTTP/HTTPS)
- **Frontend**: HTML5, CSS3, JavaScript (ES6+)
- **Security**: SSL/TLS, Self-signed Certificates
- **PWA**: Service Worker, Web App Manifest
- **반응형**: Media Queries, Mobile-First Design

## 📝 라이선스

MIT License

## 👥 개발자

- 개발: IoT Class Project
- 날짜: 2025년 10월

