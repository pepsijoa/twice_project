import socket
import os
import time
import cv2
import json
import statistics
import numpy as np
from picamera2 import Picamera2
from collections import Counter, defaultdict

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
server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
server.bind(SOCKET_PATH)
server.listen(1)

print(f"[Python] 소켓 서버 대기 중... ({SOCKET_PATH})")

try:
    # 한 번만 연결 수락 (persistent)
    conn, addr = server.accept()
    print("[Python] C++ 클라이언트 연결됨")
    
    while True:
        # 데이터 수신 (Trigger)
        data = conn.recv(1024)
        if not data:
            print("[Python] 연결이 종료되었습니다.")
            break
        
        msg = data.decode('utf-8').strip()
        
        if msg == "get_marker_data":
            # 마커 분석 수행
            json_output = get_marker_data()
            
            # 결과 전송 + 구분자
            response = json_output + "\n"
            conn.sendall(response.encode('utf-8'))
            print(f"[Python] 데이터 전송 완료: {json_output}")
        elif msg == "quit":
            print("[Python] 종료 요청 받음")
            break

except KeyboardInterrupt:
    print("\n[Python] 종료 중...")
finally:
    try:
        conn.close()
    except:
        pass
    picam2.stop()
    server.close()
    if os.path.exists(SOCKET_PATH):
        os.remove(SOCKET_PATH)