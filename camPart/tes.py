#!/usr/bin/env python3

import os
import time
from datetime import datetime
from picamera2 import Picamera2

# 저장 디렉토리 설정
SAVE_DIR = os.path.join(os.path.dirname(__file__), "captured_images")
os.makedirs(SAVE_DIR, exist_ok=True)

print("=" * 60)
print("  Picamera2 사진 촬영 프로그램")
print("=" * 60)
print(f"저장 경로: {SAVE_DIR}")
print("=" * 60 + "\n")

# 카메라 초기화
print("[1/4] 📷 카메라 초기화 중...")
picam2 = Picamera2()

# 설정 생성
print("[2/4] ⚙️  카메라 설정 중...")
config = picam2.create_still_configuration(main={"size": (1920, 1080)})
picam2.configure(config)

# 카메라 시작
print("[3/4] 🚀 카메라 시작 중...")
picam2.start()

# 카메라 워밍업
print("[4/4] ⏳ 카메라 워밍업 중 (2초)...")
time.sleep(2)

# 사진 촬영
timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
filename = f"photo_{timestamp}.jpg"
filepath = os.path.join(SAVE_DIR, filename)

print(f"\n📸 사진 촬영 중...")
picam2.capture_file(filepath)

print(f"✅ 촬영 완료!")
print(f"📁 저장 위치: {filepath}")

# 카메라 종료
picam2.stop()
print("\n🎯 카메라 종료 완료")
print("=" * 60)