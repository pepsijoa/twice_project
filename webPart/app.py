from flask import Flask, render_template, request, jsonify, send_from_directory, send_file
import os
import ssl
import socket
import time
import threading
import json
import datetime # datetime 모듈 상단으로 이동
from dotenv import load_dotenv
import database as db

# 환경 변수 로드
load_dotenv()

app = Flask(__name__)
SOCKET_PATH = "/tmp/flaskToCPP.sock"

# 소켓 통신을 위한 스레드 락
socket_lock = threading.Lock()

# 안전한 소켓 통신 헬퍼 함수
def safe_socket_communication(command, buffer_size=4096, timeout=5):
    with socket_lock: 
        try:
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
                client.settimeout(timeout)  
                client.connect(SOCKET_PATH)
                client.send(command.encode('utf-8'))
                
                data = b""
                while True:
                    try:
                        chunk = client.recv(buffer_size)
                        if not chunk:
                            break
                        data += chunk
                    except socket.timeout:
                        break # 타임아웃 시 받은 데이타까지만 처리
                
                response = data.decode('utf-8')
                return True, response
        except socket.timeout:
            return False, "소켓 연결 시간 초과"
        except FileNotFoundError:
            return False, f"({SOCKET_PATH}) 문제"
        except OSError as e:
            return False, f"소켓 연결 오류: {str(e)}"
        finally:
            time.sleep(0.05)  # 50ms로 단축
            
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
    return render_template('main.html')

# 컨트롤러 페이지 라우트
@app.route('/controller')
def controller():
    return render_template('controller.html')

# 재고 관리 페이지 라우트
@app.route('/inventory')
def inventory():
    return render_template('inventory.html')

# 재고 목록 조회 API
@app.route('/api/inventory', methods=['GET'])
def get_inventory_list():
    """모든 재고 목록 조회"""
    try:
        items = db.get_all_inventory()
        return jsonify({'status': 'success', 'items': items})
    except Exception as e:
        print(f"재고 목록 조회 오류: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

# 재고 검색 API
@app.route('/api/inventory/search', methods=['GET'])
def search_inventory_api():
    """재고 검색"""
    try:
        keyword = request.args.get('q', '')
        if keyword:
            items = db.search_inventory(keyword)
        else:
            items = db.get_all_inventory()
        return jsonify({'status': 'success', 'items': items})
    except Exception as e:
        print(f"재고 검색 오류: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

# 재고 추가 API (복원됨)
@app.route('/api/inventory', methods=['POST'])
def add_inventory_api():
    """새 재고 추가"""
    try:
        data = request.get_json()
        name = data.get('name')
        quantity = data.get('quantity', 0)
        location = data.get('location')
        
        if not name or not location:
            return jsonify({'status': 'error', 'message': '제품 이름과 위치는 필수입니다.'}), 400
        
        item_id = db.add_inventory(name, quantity, location)
        if item_id:
            return jsonify({'status': 'success', 'id': item_id, 'message': '재고가 추가되었습니다.'})
        else:
            return jsonify({'status': 'error', 'message': '재고 추가에 실패했습니다.'}), 500
    except Exception as e:
        print(f"재고 추가 오류: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

# 재고 수정 API
@app.route('/api/inventory/<int:item_id>', methods=['PUT'])
def update_inventory_api(item_id):
    """재고 정보 수정"""
    try:
        data = request.get_json()
        name = data.get('name')
        quantity = data.get('quantity')
        location = data.get('location')
        
        success = db.update_inventory(item_id, name, quantity, location)
        if success:
            return jsonify({'status': 'success', 'message': '재고가 수정되었습니다.'})
        else:
            return jsonify({'status': 'error', 'message': '재고 수정에 실패했습니다.'}), 404
    except Exception as e:
        print(f"재고 수정 오류: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

# 재고 삭제 API
@app.route('/api/inventory/<int:item_id>', methods=['DELETE'])
def delete_inventory_api(item_id):
    """재고 삭제"""
    try:
        success = db.delete_inventory(item_id)
        if success:
            return jsonify({'status': 'success', 'message': '재고가 삭제되었습니다.'})
        else:
            return jsonify({'status': 'error', 'message': '재고 삭제에 실패했습니다.'}), 404
    except Exception as e:
        print(f"재고 삭제 오류: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

# 위치별 재고 조회 API
@app.route('/api/inventory/location/<location>', methods=['GET'])
def get_inventory_by_location_api(location):
    """특정 위치의 재고 조회"""
    try:
        items = db.get_inventory_by_location(location)
        return jsonify({'status': 'success', 'items': items})
    except Exception as e:
        print(f"위치별 재고 조회 오류: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

# Service Worker 라우트
@app.route('/sw.js')
def service_worker():
    return send_from_directory('static', 'sw.js', mimetype='application/javascript')

# 방향 버튼 제어 라우트
@app.route('/control', methods=['POST'])
def control():
    data = request.get_json()
    direction = data.get('direction')
    
    success, response = safe_socket_communication(direction)
    
    if success:
        print(f"C++ 서버 응답: {response}")
        
        # MAPPINGDONE 응답 처리
        if response == "MAPPINGDONE":
            return jsonify({
                'status': 'mapping_done', 
                'direction': direction, 
                'response': response,
                'message': '현재 매핑은 모두 완료되었습니다.'
            })
        else:
            return jsonify({'status': 'success', 'direction': direction, 'response': response})
    else:
        print(f"소켓 통신 실패: {response}")
        return jsonify({'status': 'error', 'message': response})

# 매핑 완료 라우트
@app.route('/mapping-complete', methods=['POST'])
def mapping_complete():
    data = request.get_json()
    action = data.get('action')
    
    if action == 'doneMapping':
        success, response = safe_socket_communication('doneMapping')
        
        if success:
            print(f"C++ 서버 응답 (매핑 완료): {response}")
            return jsonify({'status': 'success', 'action': 'doneMapping', 'response': response})
        else:
            print(f"매핑 완료 신호 전송 실패: {response}")
            return jsonify({'status': 'error', 'message': response})
    else:
        return jsonify({'status': 'error', 'message': 'Invalid action'})

# 리매핑 라우트
@app.route('/remapping', methods=['POST'])
def remapping():
    success, response = safe_socket_communication('remapping')
    
    if success:
        print(f"C++ 서버 응답 (리매핑): {response}")
        if response == "REMAPPING_QUEUED":
            return jsonify({'status': 'success', 'action': 'remapping', 'message': '리매핑이 요청되었습니다. 잠시 후 시작됩니다.'})
        else:
            return jsonify({'status': 'success', 'action': 'remapping', 'response': response})
    else:
        print(f"리매핑 신호 전송 실패: {response}")
        return jsonify({'status': 'error', 'message': response})

# 카메라 촬영 라우트 (수정 및 정리됨)
@app.route('/camera', methods=['POST'])
def camera_shot():
    """카메라 촬영 요청 처리"""
    try:
        data = request.get_json()
        
        # 1. 이름 가져오기 및 기본값 설정
        user_input_name = data.get('name')
        if not user_input_name:
            timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
            user_input_name = f"Point_{timestamp}"
            
        # 2. 공백을 언더바(_)로 치환하여 안전한 문자열 생성
        safe_name = user_input_name.replace(" ", "_")
        
        # 3. C++로 보낼 명령어 조합
        command = f"featureShot/{safe_name}"
        print(f"📷 카메라 촬영 요청 전송: {command}")
        
        # 4. 소켓 통신
        success, response = safe_socket_communication(command, timeout=10)
        
        if not success:
            return jsonify({'status': 'error', 'message': f'전송 실패: {response}'})
            
        # 5. 응답 처리
        if response.strip() == "FEATURESHOT_OK":
            return jsonify({'status': 'success', 'message': '촬영 성공', 'name': safe_name})
        elif response.strip() == "FEATURESHOT_FAIL":
            return jsonify({'status': 'failed', 'message': '촬영 실패'})
        else:
            return jsonify({'status': 'error', 'message': f'예상치 못한 응답: {response}'})
            
    except Exception as e:
        print(f"카메라 촬영 중 오류 발생: {e}")
        return jsonify({'status': 'error', 'message': str(e)})

# 맵 데이터 가져오기 라우트
@app.route('/get-map', methods=['GET'])
def get_map():
    success, response = safe_socket_communication('requestMap', buffer_size=4096)
    
    if not success:
        print(f"맵 데이터 요청 실패: {response}")
        return jsonify({'status': 'error', 'message': response})
    
    # 응답이 유효한 JSON인지 확인
    if response and response != "NO_MAP":
        try:
            map_data = json.loads(response)
            return jsonify({'status': 'success', 'map': map_data})
        except json.JSONDecodeError:
            # JSON이 아닌 경우 기본 응답
            return jsonify({'status': 'no_map', 'message': 'Map not ready'})
    else:
        return jsonify({'status': 'no_map', 'message': 'Map not available'})

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
    # 외부 IP 주소 고정 (필요시 변경)
    ip = "112.214.181.224"
    
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
    # 데이터베이스 초기화
    print("🔧 데이터베이스 초기화 중...")
    if db.init_database():
        print("✅ 데이터베이스 준비 완료")
    else:
        print("⚠️  데이터베이스 연결 실패 - 재고 관리 기능이 제한될 수 있습니다.")
    
    # 포트 설정 (환경변수에서 가져오거나 기본값 5000)
    port = int(os.environ.get('FLASK_PORT', 5000))
    
    # SSL 인증서 파일 경로
    cert_dir = os.path.join(os.path.dirname(__file__), 'certification', 'certs')
    cert_file = os.path.join(cert_dir, 'cert.pem')
    key_file = os.path.join(cert_dir, 'key.pem')
    
    # 인증서가 있으면 HTTPS로 실행, 없으면 HTTP로 실행
    if os.path.exists(cert_file) and os.path.exists(key_file):
        print("🔒 HTTPS 모드로 서버를 시작합니다...")
        print(f"📱 접속 주소: https://<IP주소>:{port}")
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(cert_file, key_file)
        app.run(debug=True, host='0.0.0.0', port=port, ssl_context=context)
    else:
        print("⚠️  SSL 인증서가 없습니다. HTTP 모드로 서버를 시작합니다...")
        print(f"💡 HTTPS를 사용하려면: python certification/generate_cert.py 실행")
        print(f"📱 접속 주소: http://<IP주소>:{port}")
        app.run(debug=True, host='0.0.0.0', port=port)