# 📱 모바일에서 HTTPS 경고 제거 가이드

자체 서명 SSL 인증서를 모바일 기기에서 신뢰하도록 설정하는 방법입니다.

---

## 🍎 iOS (iPhone/iPad)

### 방법 1: AirDrop 사용 (Mac이 있는 경우)

1. **Mac에서 인증서 전송**
   ```bash
   # Mac에서 AirDrop으로 cert.pem 파일 전송
   open certs/cert.pem
   ```

2. **iPhone에서 프로파일 설치**
   - 설정 → 프로파일이 다운로드됨 → 설치
   - 암호 입력
   - 설치 완료

3. **인증서 신뢰 설정**
   - 설정 → 일반 → 정보 → 인증서 신뢰 설정
   - "localhost" 또는 프로젝트 이름 찾기
   - 스위치 켜기 → 계속

### 방법 2: 이메일 사용

1. **이메일로 인증서 전송**
   ```bash
   # 이메일 클라이언트에서 certs/cert.pem 첨부
   ```

2. **iPhone에서 이메일 열기**
   - 첨부파일 (cert.pem) 클릭
   - "프로파일 설치" 화면 표시
   - 설치 진행 (위와 동일)

### 방법 3: 웹 브라우저에서 다운로드

1. **서버에 인증서 다운로드 라우트 추가** (아래 참고)
2. **Safari에서 접속**
   ```
   http://<라즈베리파이IP>:5000/download-cert
   ```
3. **인증서 설치 진행** (위와 동일)

---

## 🤖 Android

### 방법 1: 직접 전송

1. **파일 전송**
   - USB, 이메일, 메신저 등으로 `certs/cert.pem` 전송
   
2. **인증서 설치**
   - 설정 → 보안 → 고급 → 암호화 및 자격 증명
   - "인증서 설치" → "CA 인증서"
   - 파일 선택: cert.pem
   - 이름 입력: "IoT Controller"
   - 확인

### 방법 2: Chrome에서 설치

1. **Chrome 설정**
   - chrome://settings/security
   - "인증서 관리"
   - "사용자" 탭

2. **인증서 가져오기**
   - 메뉴 (⋮) → 가져오기
   - cert.pem 선택
   - 모든 옵션 체크
   - 확인

### 방법 3: 웹에서 다운로드

1. **브라우저에서 접속**
   ```
   http://<라즈베리파이IP>:5000/download-cert
   ```
   
2. **다운로드된 파일 설치**
   - 알림에서 "인증서 설치" 클릭
   - 위 방법 1 진행

---

## 🌐 웹 브라우저별 설정

### Chrome (PC)

1. **설정 열기**
   ```
   chrome://settings/certificates
   ```

2. **인증서 가져오기**
   - "기관" 탭 선택
   - "가져오기" 클릭
   - `certs/cert.pem` 선택
   - 모든 옵션 체크
   - 확인

3. **브라우저 재시작**

### Firefox (PC)

1. **설정 열기**
   ```
   about:preferences#privacy
   ```

2. **인증서 보기**
   - "인증서" 섹션
   - "인증서 보기" 클릭

3. **인증서 가져오기**
   - "기관" 탭
   - "가져오기" 클릭
   - `certs/cert.pem` 선택
   - "이 CA를 신뢰하여 웹 사이트 식별" 체크
   - 확인

### Edge (PC)

Chrome과 동일 (Chromium 기반)

---

## 🔧 서버에 인증서 다운로드 기능 추가

`app.py`에 다음 라우트를 추가하면 모바일에서 쉽게 인증서를 다운로드할 수 있습니다:

```python
from flask import send_file

@app.route('/download-cert')
def download_cert():
    """인증서 다운로드 페이지"""
    cert_path = os.path.join(os.path.dirname(__file__), 'certs', 'cert.pem')
    if os.path.exists(cert_path):
        return send_file(
            cert_path,
            as_attachment=True,
            download_name='iot-controller-cert.pem',
            mimetype='application/x-pem-file'
        )
    return "인증서 파일을 찾을 수 없습니다.", 404

@app.route('/cert-guide')
def cert_guide():
    """인증서 설치 가이드 페이지"""
    return '''
    <html>
    <body style="font-family: sans-serif; padding: 20px;">
        <h1>📱 인증서 설치 가이드</h1>
        <h2>iOS</h2>
        <ol>
            <li><a href="/download-cert">인증서 다운로드</a></li>
            <li>설정 → 프로파일이 다운로드됨 → 설치</li>
            <li>설정 → 일반 → 정보 → 인증서 신뢰 설정</li>
            <li>스위치 켜기</li>
        </ol>
        <h2>Android</h2>
        <ol>
            <li><a href="/download-cert">인증서 다운로드</a></li>
            <li>설정 → 보안 → 인증서 설치 → CA 인증서</li>
            <li>다운로드한 파일 선택</li>
        </ol>
    </body>
    </html>
    '''
```

그런 다음:
1. HTTP로 접속: `http://<IP>:5000/cert-guide`
2. 인증서 다운로드 및 설치
3. HTTPS로 재접속: `https://<IP>:5000`

---

## ⚠️ 주의사항

1. **자체 서명 인증서의 한계**
   - 완전한 경고 제거는 어려울 수 있습니다
   - 각 기기마다 수동 설정 필요

2. **보안**
   - 로컬 네트워크에서만 사용 권장
   - 외부 인터넷에 노출하지 마세요

3. **대안: Let's Encrypt**
   - 공용 도메인이 있다면 무료 SSL 인증서 사용
   - Certbot으로 자동 발급 가능

---

## 💡 가장 쉬운 방법

**경고를 그냥 수락하는 것이 가장 간단합니다!**

로컬 개발 환경에서는:
- "고급" → "계속 진행" 클릭
- 매번 접속할 때마다 한 번씩만 수락하면 됩니다
- 브라우저가 선택을 기억합니다

프로덕션 환경이 아니라면 이 방법이 가장 실용적입니다.
