from flask import Flask, render_template, request, jsonify, send_from_directory, send_file
import os
import ssl

app = Flask(__name__)

# 보안 헤더 추가
@app.after_request
def add_security_headers(response):
    response.headers['X-Content-Type-Options'] = 'nosniff'
    response.headers['X-Frame-Options'] = 'SAMEORIGIN'
    response.headers['X-XSS-Protection'] = '1; mode=block'
    # HTTPS 강제 (Strict-Transport-Security)
    if request.is_secure:
        response.headers['Strict-Transport-Security'] = 'max-age=31536000; includeSubDomains'
    return response

# 메인 페이지 라우트
@app.route('/')
def index():
    return render_template('index.html')

# Service Worker 라우트
@app.route('/sw.js')
def service_worker():
    return send_from_directory('static', 'sw.js', mimetype='application/javascript')

# 방향 버튼 제어 라우트
@app.route('/control', methods=['POST'])
def control():
    data = request.get_json()
    direction = data.get('direction')
    
    # 방향에 따른 처리
    if direction == 'up':
        print("⬆️ 위쪽 버튼이 클릭되었습니다!")
    elif direction == 'down':
        print("⬇️ 아래쪽 버튼이 클릭되었습니다!")
    elif direction == 'left':
        print("⬅️ 왼쪽 버튼이 클릭되었습니다!")
    elif direction == 'right':
        print("➡️ 오른쪽 버튼이 클릭되었습니다!")
    else:
        print("❌ 알 수 없는 방향입니다.")
        return jsonify({'status': 'error', 'message': '잘못된 방향입니다.'}), 400
    
    return jsonify({'status': 'success', 'direction': direction})

# 인증서 다운로드 라우트
@app.route('/download-cert')
def download_cert():
    """SSL 인증서 다운로드"""
    cert_path = os.path.join(os.path.dirname(__file__), 'certification', 'certs', 'cert.pem')
    if os.path.exists(cert_path):
        return send_file(
            cert_path,
            as_attachment=True,
            download_name='iot-controller-cert.pem',
            mimetype='application/x-pem-file'
        )
    return "인증서 파일을 찾을 수 없습니다. 먼저 'python certification/generate_cert.py'를 실행하세요.", 404

# 인증서 설치 가이드 페이지
@app.route('/cert-guide')
def cert_guide():
    """인증서 설치 가이드"""
    import socket
    hostname = socket.gethostname()
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
    except:
        ip = "127.0.0.1"
    
    return f'''
    <!DOCTYPE html>
    <html lang="ko">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>📱 인증서 설치 가이드</title>
        <style>
            body {{
                font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
                padding: 20px;
                max-width: 800px;
                margin: 0 auto;
                background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                color: white;
            }}
            .container {{
                background: white;
                color: #333;
                padding: 30px;
                border-radius: 15px;
                box-shadow: 0 10px 30px rgba(0,0,0,0.3);
            }}
            h1 {{ color: #667eea; }}
            h2 {{ color: #764ba2; margin-top: 30px; }}
            .download-btn {{
                display: inline-block;
                background: linear-gradient(145deg, #667eea, #764ba2);
                color: white;
                padding: 15px 30px;
                text-decoration: none;
                border-radius: 10px;
                font-size: 18px;
                font-weight: bold;
                margin: 20px 0;
                box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
            }}
            .download-btn:hover {{
                transform: translateY(-2px);
                box-shadow: 0 8px 20px rgba(102, 126, 234, 0.6);
            }}
            ol {{ line-height: 2; }}
            .info-box {{
                background: #f0f0f0;
                padding: 15px;
                border-radius: 10px;
                margin: 15px 0;
                border-left: 4px solid #667eea;
            }}
            code {{
                background: #f5f5f5;
                padding: 2px 6px;
                border-radius: 3px;
                font-family: monospace;
            }}
        </style>
    </head>
    <body>
        <div class="container">
            <h1>📱 SSL 인증서 설치 가이드</h1>
            
            <div class="info-box">
                <strong>🌐 현재 서버 정보:</strong><br>
                호스트명: {hostname}<br>
                IP 주소: {ip}<br>
                포트: 5000
            </div>
            
            <a href="/download-cert" class="download-btn">📥 인증서 다운로드</a>
            
            <h2>🍎 iOS (iPhone/iPad)</h2>
            <ol>
                <li>위 버튼을 클릭하여 <strong>인증서 다운로드</strong></li>
                <li><strong>설정</strong> → <strong>프로파일이 다운로드됨</strong> (또는 일반)</li>
                <li>다운로드한 프로파일 선택 후 <strong>설치</strong></li>
                <li>기기 암호 입력</li>
                <li><strong>설정</strong> → <strong>일반</strong> → <strong>정보</strong> → <strong>인증서 신뢰 설정</strong></li>
                <li>"IoT Controller" 또는 "localhost" 찾아서 <strong>스위치 켜기</strong></li>
                <li>경고 확인 후 <strong>계속</strong></li>
            </ol>
            
            <h2>🤖 Android</h2>
            <ol>
                <li>위 버튼을 클릭하여 <strong>인증서 다운로드</strong></li>
                <li><strong>설정</strong> → <strong>보안</strong> (또는 잠금 화면 및 보안)</li>
                <li><strong>고급</strong> → <strong>암호화 및 자격 증명</strong></li>
                <li><strong>인증서 설치</strong> → <strong>CA 인증서</strong></li>
                <li>다운로드한 파일 선택: <code>iot-controller-cert.pem</code></li>
                <li>인증서 이름 입력: "IoT Controller"</li>
                <li><strong>확인</strong></li>
            </ol>
            
            <h2>💻 PC (Chrome/Edge)</h2>
            <ol>
                <li>위 버튼을 클릭하여 <strong>인증서 다운로드</strong></li>
                <li>주소창에 입력: <code>chrome://settings/certificates</code></li>
                <li><strong>기관</strong> 탭 선택</li>
                <li><strong>가져오기</strong> 클릭</li>
                <li>다운로드한 파일 선택</li>
                <li>모든 옵션 체크 후 <strong>확인</strong></li>
                <li><strong>브라우저 재시작</strong></li>
            </ol>
            
            <div class="info-box">
                <strong>✅ 설치 완료 후:</strong><br>
                HTTPS로 재접속하세요:<br>
                <code>https://{ip}:5000</code>
            </div>
            
            <h2>⚠️ 주의사항</h2>
            <ul>
                <li>자체 서명 인증서이므로 일부 경고는 여전히 표시될 수 있습니다</li>
                <li>로컬 네트워크에서만 사용하세요</li>
                <li>각 기기마다 인증서를 설치해야 합니다</li>
            </ul>
            
            <h2>💡 더 쉬운 방법</h2>
            <p>개발 환경에서는 그냥 <strong>"고급" → "계속 진행"</strong>을 클릭하는 것이 가장 간단합니다!</p>
            
            <div style="margin-top: 30px; text-align: center;">
                <a href="/" style="color: #667eea; text-decoration: none; font-weight: bold;">← 홈으로 돌아가기</a>
            </div>
        </div>
    </body>
    </html>
    '''

if __name__ == '__main__':
    # SSL 인증서 파일 경로
    cert_dir = os.path.join(os.path.dirname(__file__), 'certification', 'certs')
    cert_file = os.path.join(cert_dir, 'cert.pem')
    key_file = os.path.join(cert_dir, 'key.pem')
    
    # 인증서가 있으면 HTTPS로 실행, 없으면 HTTP로 실행
    if os.path.exists(cert_file) and os.path.exists(key_file):
        print("🔒 HTTPS 모드로 서버를 시작합니다...")
        print(f"📱 접속 주소: https://<IP주소>:5000")
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(cert_file, key_file)
        app.run(debug=True, host='0.0.0.0', port=5000, ssl_context=context)
    else:
        print("⚠️  SSL 인증서가 없습니다. HTTP 모드로 서버를 시작합니다...")
        print(f"💡 HTTPS를 사용하려면: python certification/generate_cert.py 실행")
        print(f"📱 접속 주소: http://<IP주소>:5000")
        app.run(debug=True, host='0.0.0.0', port=5000)
