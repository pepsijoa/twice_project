# 📁 프로젝트 구조 변경 사항

## 🔄 변경 내용

인증서 관련 파일들을 `certification` 디렉토리로 정리했습니다.

### 이동된 파일들

```
이전 위치                    → 새 위치
─────────────────────────────────────────────────────────
generate_cert.py            → certification/generate_cert.py
generate_cert_advanced.py   → certification/generate_cert_advanced.py
trust_cert.sh               → certification/trust_cert.sh
certs/                      → certification/certs/
MOBILE_CERT_GUIDE.md        → certification/MOBILE_CERT_GUIDE.md
REMOVE_WARNING.md           → certification/REMOVE_WARNING.md
```

### 수정된 파일들

✅ **app.py**
- 인증서 경로: `certs/` → `certification/certs/`
- 에러 메시지 업데이트

✅ **start.sh**
- 인증서 확인 경로 업데이트
- generate_cert.py 실행 경로 수정

✅ **.gitignore**
- certs/ → certification/certs/

✅ **README.md**
- 문서 링크 업데이트
- 프로젝트 구조 다이어그램 수정
- 명령어 예제 업데이트

✅ **QUICKSTART.md**
- 명령어 경로 업데이트

---

## 📁 새로운 프로젝트 구조

```
twiceProjeect/
├── app.py                      # 메인 Flask 애플리케이션
├── start.sh                    # 서버 시작 스크립트
├── README.md                   # 프로젝트 문서
├── QUICKSTART.md               # 빠른 시작 가이드
│
├── certification/              # 🆕 인증서 관련 파일 모음
│   ├── generate_cert.py        # 기본 인증서 생성
│   ├── generate_cert_advanced.py # 고급 인증서 생성 (SAN 지원)
│   ├── trust_cert.sh           # 시스템 신뢰 추가 스크립트
│   ├── certs/                  # 생성된 인증서 저장소
│   │   ├── cert.pem
│   │   ├── key.pem
│   │   └── openssl.cnf
│   ├── REMOVE_WARNING.md       # HTTPS 경고 제거 가이드
│   └── MOBILE_CERT_GUIDE.md    # 모바일 인증서 설치 가이드
│
├── templates/
│   └── index.html              # HTML 템플릿
│
├── static/
│   ├── css/
│   │   └── style.css
│   ├── js/
│   │   └── script.js
│   ├── images/
│   ├── manifest.json
│   └── sw.js
│
└── venv/                       # Python 가상환경
```

---

## 🚀 사용 방법 (변경됨)

### 인증서 생성

**이전:**
```bash
python generate_cert.py
```

**현재:**
```bash
python certification/generate_cert.py
```

### 고급 인증서 생성

**이전:**
```bash
python generate_cert_advanced.py
```

**현재:**
```bash
python certification/generate_cert_advanced.py
```

### 시스템 신뢰 추가

**이전:**
```bash
sudo ./trust_cert.sh
```

**현재:**
```bash
cd certification
sudo ./trust_cert.sh
```

또는 프로젝트 루트에서:
```bash
(cd certification && sudo ./trust_cert.sh)
```

### 서버 시작 (변경 없음)

```bash
# 방법 1: 자동 스크립트 (권장)
./start.sh

# 방법 2: 직접 실행
python app.py
```

---

## 📖 문서 참조

### 경로가 변경된 문서

**HTTPS 경고 제거:**
- 이전: `REMOVE_WARNING.md`
- 현재: `certification/REMOVE_WARNING.md`

**모바일 인증서 설치:**
- 이전: `MOBILE_CERT_GUIDE.md`
- 현재: `certification/MOBILE_CERT_GUIDE.md`

### README 링크도 자동 업데이트됨

```markdown
- 🔒 [HTTPS 경고 제거](certification/REMOVE_WARNING.md)
- 📱 [모바일 인증서 설치](certification/MOBILE_CERT_GUIDE.md)
```

---

## ✅ 호환성

### 영향받지 않는 기능

- ✅ 서버 시작 (`python app.py` 또는 `./start.sh`)
- ✅ 웹 브라우저 접속
- ✅ 버튼 클릭 기능
- ✅ HTTPS/HTTP 자동 전환
- ✅ 인증서 다운로드 (`/download-cert`)
- ✅ 인증서 가이드 페이지 (`/cert-guide`)

### 주의사항

기존에 `certs/` 디렉토리에 인증서를 생성했다면:
```bash
# 기존 인증서 이동 (이미 완료됨)
# mv certs certification/
```

---

## 🎯 변경 이유

### 장점

1. **🗂️ 더 깔끔한 구조**
   - 루트 디렉토리가 간결해짐
   - 관련 파일들이 논리적으로 그룹화됨

2. **📚 문서 관리 용이**
   - 인증서 관련 문서가 함께 위치
   - 찾기 쉬움

3. **🔧 유지보수 개선**
   - 인증서 관련 파일 한 곳에서 관리
   - 백업/배포 시 편리

4. **🎓 학습 효과**
   - 프로젝트 구조의 모범 사례 학습
   - 실무 프로젝트 구조와 유사

---

## 🔍 검증

### 테스트 완료 항목

✅ app.py 로드 테스트
✅ 경로 참조 확인
✅ start.sh 스크립트 동작
✅ .gitignore 업데이트
✅ 문서 링크 검증

### 확인 방법

```bash
# 프로젝트 구조 확인
tree -L 2 -I 'venv|__pycache__'

# 앱 로드 테스트
python -c "from app import app; print('OK')"

# 인증서 생성 테스트
python certification/generate_cert.py
```

---

## 💡 추가 정보

변경 사항에 대한 질문이나 문제가 있다면:
1. README.md의 업데이트된 섹션 참조
2. QUICKSTART.md의 새로운 명령어 확인
3. certification/ 디렉토리의 문서 참조

---

**변경 일자**: 2025년 10월 30일  
**상태**: ✅ 완료 및 테스트됨
