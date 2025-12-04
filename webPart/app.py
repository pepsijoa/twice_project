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
SOCKET_PATH = "/tmp/flaskToCPP.sock"  # Flask -> C++ 통신용
CPP_EVENT_SOCKET = "/tmp/cppToFlask.sock"  # C++ -> Flask 이벤트 수신용

# 소켓 통신을 위한 스레드 락
socket_lock = threading.Lock()

# C++ 이벤트 수신 서버 스레드
def cpp_event_listener():
    """
    C++에서 보내는 이벤트를 수신하는 별도 소켓 서버
    """
    # 기존 소켓 파일 제거
    if os.path.exists(CPP_EVENT_SOCKET):
        os.unlink(CPP_EVENT_SOCKET)
    
    # Unix 소켓 서버 생성
    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server.bind(CPP_EVENT_SOCKET)
    server.listen(5)
    print(f"✅ C++ 이벤트 수신 서버 시작: {CPP_EVENT_SOCKET}")
    
    while True:
        try:
            conn, _ = server.accept()
            data = conn.recv(4096).decode('utf-8').strip()
            
            if data.startswith("EVENT:"):
                print(f"📥 C++ 이벤트 수신: {data[:50]}...")
                handle_cpp_event(data)
                conn.send(b"OK\n")
            else:
                conn.send(b"ERROR:Invalid format\n")
            
            conn.close()
        except Exception as e:
            print(f"❌ C++ 이벤트 수신 오류: {e}")

# C++ 이벤트 리스너 스레드 시작
event_listener_thread = threading.Thread(target=cpp_event_listener, daemon=True)
event_listener_thread.start()

# 안전한 소켓 통신 헬퍼 함수
def safe_socket_communication(command, buffer_size=4096, timeout=30, max_retries=3):
    with socket_lock: 
        for attempt in range(max_retries):
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
                    
                    # C++에서 보낸 EVENT 메시지 처리
                    if response.startswith("EVENT:"):
                        handle_cpp_event(response)
                        return True, "EVENT_PROCESSED"
                    
                    return True, response
                    
            except socket.timeout:
                if attempt < max_retries - 1:
                    time.sleep(0.1 * (attempt + 1))  # 점진적 대기
                    continue
                return False, "소켓 연결 시간 초과"
                
            except FileNotFoundError:
                return False, f"소켓 파일 없음 ({SOCKET_PATH})"
                
            except OSError as e:
                # Errno 11 (Resource temporarily unavailable) 재시도
                if e.errno == 11 and attempt < max_retries - 1:
                    time.sleep(0.05 * (attempt + 1))
                    continue
                return False, f"소켓 연결 오류: {str(e)}"
                
            finally:
                time.sleep(0.02)  # 20ms로 단축

# --- Map cache + rate limit for /get-map ----------------------------------
# simple in-memory cache to avoid hammering the C++ socket when clients poll
map_cache = None
map_cache_ts = 0.0
map_cache_lock = threading.Lock()
MAP_TTL_SECONDS = 3.0       # 캐시 유효 시간 (초)
RATE_LIMIT_SECONDS = 0.8   # 동일 클라이언트의 최소 요청 간격 (초)
last_request_times = {}     # {client_ip: last_time}

def refresh_map_from_cpp():
    """Background helper: request map from C++ and update cache if valid."""
    global map_cache, map_cache_ts
    try:
        success, response = safe_socket_communication('requestMap', buffer_size=8192, timeout=15, max_retries=2)
        if not success:
            print(f"❌ map refresh 실패: {response}")
            return

        if not response or response == "NO_MAP":
            print("❌ map refresh: NO_MAP or empty")
            return

        try:
            parsed = json.loads(response)
        except Exception as e:
            print(f"❌ map refresh: JSON 파싱 실패: {e}")
            return

        with map_cache_lock:
            map_cache = parsed
            map_cache_ts = time.time()
        print("✅ map cache updated from C++")

    except Exception as e:
        print(f"❌ refresh_map_from_cpp 예외: {e}")

# C++에서 보낸 이벤트 처리
def handle_cpp_event(event_message):
    """
    C++에서 보낸 EVENT 메시지 처리
    형식: EVENT:eventType:jsonPayload
    """
    try:
        parts = event_message.split(":", 2)  # 최대 3개로 분할
        if len(parts) < 3:
            print(f"⚠️ 잘못된 이벤트 형식: {event_message}")
            return
        
        event_type = parts[1]
        json_payload = parts[2]
        
        print(f"🎯 C++ 이벤트 수신: {event_type}")
        
        if event_type == "inventory_update":
            # JSON 파싱
            payload = json.loads(json_payload)
            inventory_id = payload.get('inventoryID')  # 제품 이름(정수)
            feature_name = payload.get('featureName')  # 위치 (location)
            box_count = payload.get('boxCount')
            
            if inventory_id is not None and feature_name and box_count is not None:
                print(f"📊 재고 업데이트 요청: 제품ID={inventory_id}, 위치={feature_name}, 수량={box_count}")
                
                # EVENT는 신뢰할 수 있으므로 무조건 DB 업데이트
                try:
                    # 먼저 모든 재고 조회 (디버깅용)
                    all_items = db.get_all_inventory()
                    print(f"🔍 현재 DB 재고 목록: {[(item['name'], item['location'], item['id']) for item in all_items]}")
                    
                    # 같은 제품명(name)과 위치(location)를 가진 재고 찾기
                    item = None
                    for existing_item in all_items:
                        if existing_item['name'] == str(inventory_id) and existing_item['location'] == feature_name:
                            item = existing_item
                            break
                    
                    print(f"🔎 조회 결과: name={str(inventory_id)}, location={feature_name} → {item}")
                    
                    if item:
                        # 기존 재고가 있으면 수량만 업데이트
                        print(f"📝 업데이트 시도: ID={item['id']}, quantity={box_count}")
                        success = db.update_inventory(
                            item['id'],
                            quantity=box_count
                        )
                        print(f"✅ 업데이트 결과: {success}")
                        
                        if success:
                            print(f"✅ 재고 업데이트 완료: 제품={inventory_id}, 위치={feature_name}, 수량={box_count}개")
                        else:
                            print(f"❌ 재고 업데이트 실패: 제품ID={inventory_id}")
                    else:
                        # 해당 위치에 재고가 없으면 새로 생성
                        print(f"🆕 새 재고 생성 시도: name={inventory_id}, location={feature_name}, quantity={box_count}")
                        new_id = db.add_inventory(
                            name=str(inventory_id),
                            quantity=box_count,
                            location=feature_name
                        )
                        if new_id:
                            print(f"✅ 새 재고 생성 완료: ID={new_id}, 제품={inventory_id}, 위치={feature_name}, 수량={box_count}개")
                        else:
                            print(f"❌ 새 재고 생성 실패: 제품ID={inventory_id}")
                        
                except Exception as db_error:
                    print(f"❌ 데이터베이스 오류: {db_error}")
            else:
                print(f"⚠️ 필수 필드 누락: inventoryID={inventory_id}, featureName={feature_name}, boxCount={box_count}")
        
        else:
            print(f"⚠️ 알 수 없는 이벤트 타입: {event_type}")
    
    except json.JSONDecodeError as e:
        print(f"❌ JSON 파싱 오류: {e}")
    except Exception as e:
        print(f"❌ 이벤트 처리 오류: {e}")
            
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

# Favicon 라우트
@app.route('/favicon.ico')
def favicon():
    return send_from_directory(
        os.path.join(app.root_path, 'static'),
        'favicon.ico',
        mimetype='image/vnd.microsoft.icon'
    )

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
        elif response == "MOVEFAIL":
            return jsonify({
                'status': 'move_fail', 
                'direction': direction, 
                'response': response,
                'message': '로봇이 이동할 수 없습니다. 장애물을 확인하세요.'
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
    # Rate-limit by client IP to avoid excessive polling
    client = request.remote_addr or 'local'
    now = time.time()
    last = last_request_times.get(client, 0.0)
    if now - last < RATE_LIMIT_SECONDS:
        return jsonify({'status': 'error', 'message': 'rate_limited'}), 429
    last_request_times[client] = now

    # Check cache
    with map_cache_lock:
        cache_copy = map_cache
        cache_age = now - map_cache_ts if map_cache_ts > 0 else float('inf')

    if cache_copy is not None and cache_age < MAP_TTL_SECONDS:
        # If cache is getting old, refresh in background but still return cached map
        if cache_age >= (MAP_TTL_SECONDS * 0.6):
            threading.Thread(target=refresh_map_from_cpp, daemon=True).start()
        return jsonify({'status': 'success', 'map': cache_copy})

    # No valid cache: start background refresh and return 202 if nothing to serve
    threading.Thread(target=refresh_map_from_cpp, daemon=True).start()

    if cache_copy is not None:
        return jsonify({'status': 'success_cached', 'map': cache_copy, 'message': 'map is stale, refresh started'})
    else:
        return jsonify({'status': 'no_map', 'message': 'Map is being prepared'}), 202

# Feature 목록 가져오기 API
@app.route('/api/features', methods=['GET'])
def get_features():
    """맵에서 feature(특징점) 목록 추출"""
    try:
        # Try to use cached map first
        with map_cache_lock:
            cache_copy = map_cache
            cache_age = time.time() - map_cache_ts if map_cache_ts > 0 else float('inf')

        if cache_copy is not None and cache_age < MAP_TTL_SECONDS:
            # parse features from cache
            if isinstance(cache_copy, dict) and 'features' in cache_copy:
                feature_names = [f.get('name', f'Feature_{i+1}') for i, f in enumerate(cache_copy['features'])]
                return jsonify({'status': 'success', 'features': feature_names})
            else:
                return jsonify({'status': 'no_features', 'message': '등록된 특징점이 없습니다.', 'features': []})

        # No valid cache: trigger background refresh and return no_map or stale cache
        threading.Thread(target=refresh_map_from_cpp, daemon=True).start()

        if cache_copy is not None:
            # return stale features if possible
            try:
                if isinstance(cache_copy, dict) and 'features' in cache_copy:
                    feature_names = [f.get('name', f'Feature_{i+1}') for i, f in enumerate(cache_copy['features'])]
                    return jsonify({'status': 'success_cached', 'features': feature_names, 'message': 'stale'})
            except Exception:
                pass

        return jsonify({'status': 'no_map', 'message': '맵이 생성되지 않았습니다.', 'features': []})
            
    except Exception as e:
        print(f"Feature 목록 조회 오류: {e}")
        return jsonify({'status': 'error', 'message': str(e)})


    
# 로봇을 특정 위치로 이동시키는 라우트
@app.route('/move-to', methods=['POST'])
def move_to_location():
    """로봇을 특정 feature 위치로 이동 (실시간 업데이트)"""
    try:
        data = request.get_json()
        location = data.get('location')

        command = f"MoveTo/{location}"
        print(f"🚀 이동 요청: {command}")
        
        # 더 긴 타임아웃과 큰 버퍼로 모든 응답 수신
        success, full_response = safe_socket_communication(command, buffer_size=8192, timeout=30)
        
        if not success:
            print(f"❌ 소켓 통신 실패: {full_response}")
            return jsonify({'status': 'error', 'message': f'전송 실패: {full_response}'})
        
        print(f"📥 C++ 서버 원본 응답:\n{full_response}")
        print(f"📏 응답 길이: {len(full_response)} bytes")
        
        # 여러 응답을 파싱
        responses = [r.strip() for r in full_response.split('\n') if r.strip()]
        print(f"📦 파싱된 응답 개수: {len(responses)}")
        
        steps = []
        total_steps = 0
        status = 'unknown'
        error_message = ''
        
        for idx, response in enumerate(responses):
            print(f"  [{idx}] {response}")
            
            if response.startswith("MoveToSTART:"):
                total_steps = int(response.split(':')[1])
                status = 'in_progress'
                print(f"    → 시작: 총 {total_steps}단계")
            elif response.startswith("MoveToOK:"):
                parts = response.split(':')
                direction = parts[1] if len(parts) > 1 else 'unknown'
                step_num = int(parts[2]) if len(parts) > 2 else 0
                steps.append({'direction': direction, 'step': step_num, 'success': True})
                print(f"    → 성공: {direction} (단계 {step_num})")
            elif response.startswith("MoveToFAIL:"):
                parts = response.split(':')
                error_message = parts[1] if len(parts) > 1 else '이동 실패'
                status = 'failed'
                print(f"    → 실패: {error_message}")
                break
            elif response == "MoveToDONE":
                status = 'completed'
                print(f"    → 완료!")
        
        # status가 여전히 'in_progress'인 경우, 모든 단계가 성공했으면 'completed'로 변경
        if status == 'in_progress' and len(steps) == total_steps and total_steps > 0:
            status = 'completed'
            print(f"✅ 모든 단계 완료로 판단: {len(steps)}/{total_steps}")
        
        result = {
            'status': status,
            'message': f'{location} 위치로 이동 {"완료" if status == "completed" else "실패" if status == "failed" else "진행중"}',
            'location': location,
            'total_steps': total_steps,
            'completed_steps': len(steps),
            'steps': steps,
            'error': error_message if error_message else None
        }
        
        print(f"📤 최종 응답: status={status}, steps={len(steps)}/{total_steps}")
        return jsonify(result)
            
    except Exception as e:
        print(f"❌ 로봇 이동 중 오류 발생: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({'status': 'error', 'message': str(e)})

# 네비게이션 완료 후 Searching 모드로 전환
@app.route('/navigatedone', methods=['POST'])
def navigate_done():
    """로봇 이동 완료 후 Searching 모드로 전환"""
    try:
        success, response = safe_socket_communication('navigatedone', timeout=10)
        
        if not success:
            print(f"❌ navigatedone 소켓 통신 실패: {response}")
            return jsonify({'status': 'error', 'message': f'전송 실패: {response}'})
        
        print(f"📥 navigatedone 응답: {response}")
        
        if response.strip() == "OK":
            return jsonify({
                'status': 'success', 
                'message': 'Searching 모드로 전환되었습니다.',
                'mode': 'searching'
            })
        else:
            return jsonify({
                'status': 'error', 
                'message': f'예상치 못한 응답: {response}'
            })
            
    except Exception as e:
        print(f"❌ navigatedone 처리 중 오류: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({'status': 'error', 'message': str(e)})

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