import socket
import os
import time
import cv2
import json
import statistics
import numpy as np
from picamera2 import Picamera2
from collections import Counter, defaultdict
import select
import signal
import sys
import atexit

# --- 소켓 설정 ---
SOCKET_PATH = "/tmp/aruco_socket"

# 기존 소켓 파일이 있다면 삭제 (충돌 방지)
if os.path.exists(SOCKET_PATH):
    os.remove(SOCKET_PATH)

# --- 카메라 및 ArUco 초기화 (한 번만 실행됨) ---
print("[Python] 카메라 초기화 중...")
picam2 = Picamera2()
config = picam2.create_video_configuration(main={"size": (640, 480), "format": "BGR888"})
picam2.configure(config)
picam2.start()

aruco_dict = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_4X4_250)
parameters = cv2.aruco.DetectorParameters()

# --- 전역 리소스 관리 ---
server = None
conn = None

def cleanup_resources():
    """모든 리소스 정리 함수"""
    global server, conn, picam2
    
    print("\n[Python] 🧹 리소스 정리 시작...")
    
    # 연결 종료
    if conn:
        try:
            conn.close()
            print("[Python] ✅ 클라이언트 연결 종료")
        except:
            pass
    
    # 서버 소켓 종료
    if server:
        try:
            server.close()
            print("[Python] ✅ 서버 소켓 종료")
        except:
            pass
    
    # 소켓 파일 삭제
    if os.path.exists(SOCKET_PATH):
        try:
            os.remove(SOCKET_PATH)
            print(f"[Python] ✅ 소켓 파일 삭제: {SOCKET_PATH}")
        except:
            pass
    
    # 카메라 종료
    try:
        picam2.stop()
        print("[Python] ✅ 카메라 종료")
    except:
        pass
    
    print("[Python] 🎯 정리 완료")

def signal_handler(signum, frame):
    """시그널 핸들러 (SIGTERM, SIGINT 등)"""
    signal_name = signal.Signals(signum).name
    print(f"\n[Python] ⚠️ 시그널 수신: {signal_name}")
    cleanup_resources()
    sys.exit(0)

# 시그널 핸들러 등록
signal.signal(signal.SIGTERM, signal_handler)  # kill 명령
signal.signal(signal.SIGINT, signal_handler)   # Ctrl+C
signal.signal(signal.SIGHUP, signal_handler)   # 터미널 종료

# atexit 핸들러 등록 (프로세스 종료 시 자동 실행)
atexit.register(cleanup_resources)

def get_marker_data():
    """1초간 10프레임 분석하여 신뢰도 높은 데이터 반환"""
    observations = defaultdict(list)
    steps = 10
    delay = 1.0 / steps
    
    for _ in range(steps):
        frame = picam2.capture_array()
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        corners, ids, rejected = cv2.aruco.detectMarkers(gray, aruco_dict, parameters=parameters)
        
        current_counts = Counter()
        if ids is not None:
            current_counts = Counter(ids.flatten().tolist())
        
        for marker_id, count in current_counts.items():
            observations[marker_id].append(count)
        time.sleep(delay)

    final_result = {}
    for marker_id, count_list in observations.items():
        zeros_to_add = steps - len(count_list)
        full_list = count_list + [0] * zeros_to_add
        reliable_count = int(statistics.median(full_list))
        
        if reliable_count > 0:
            final_result[str(marker_id)] = reliable_count
            
    return json.dumps(final_result)

# --- 메인 서버 루프 ---
print(f"[Python] 🚀 소켓 서버 시작 중... ({SOCKET_PATH})")

server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
server.bind(SOCKET_PATH)
server.listen(1)

print(f"[Python] ✅ 소켓 서버 대기 중... ({SOCKET_PATH})")

try:
    while True:
        # 연결이 없으면 새로 수락
        if conn is None:
            print("[Python] 클라이언트 연결 대기...")
            server.settimeout(1.0)  # 1초 타임아웃으로 Ctrl+C 가능하게
            try:
                conn, addr = server.accept()
                conn.settimeout(5.0)  # 데이터 수신 타임아웃 5초
                print("[Python] C++ 클라이언트 연결됨")
            except socket.timeout:
                continue
        
        try:
            # select를 사용하여 타임아웃 처리
            ready = select.select([conn], [], [], 1.0)
            
            if not ready[0]:
                # 타임아웃: 데이터가 없음 (정상, 계속 대기)
                continue
            
            # 데이터 수신
            data = conn.recv(1024)
            
            if not data:
                # 연결이 정상적으로 종료됨
                print("[Python] 클라이언트 연결 종료")
                conn.close()
                conn = None
                continue
            
            msg = data.decode('utf-8').strip()
            
            if msg == "get_marker_data":
                # 마커 분석 수행
                print("[Python] 마커 분석 요청 받음")
                json_output = get_marker_data()
                
                # 결과 전송 + 구분자
                response = json_output + "\n"
                conn.sendall(response.encode('utf-8'))
                print(f"[Python] 데이터 전송 완료: {json_output}")
                
            elif msg == "quit":
                print("[Python] 종료 요청 받음")
                conn.close()
                conn = None
                
        except socket.timeout:
            # 수신 타임아웃: 클라이언트가 응답 없음
            print("[Python] ⚠️ 클라이언트 응답 없음 (5초 타임아웃)")
            conn.close()
            conn = None
            
        except (ConnectionResetError, BrokenPipeError) as e:
            # 연결이 비정상 종료됨
            print(f"[Python] ❌ 연결 오류: {e}")
            if conn:
                conn.close()
            conn = None
            
        except Exception as e:
            # 기타 예외 처리
            print(f"[Python] ❌ 예외 발생: {e}")
            if conn:
                conn.close()
            conn = None

except KeyboardInterrupt:
    print("\n[Python] ⌨️ Ctrl+C 감지 - 정상 종료")
except Exception as e:
    print(f"\n[Python] ❌ 치명적 오류: {e}")
    import traceback
    traceback.print_exc()
finally:
    # cleanup_resources()는 atexit에서 자동 호출됨
    pass