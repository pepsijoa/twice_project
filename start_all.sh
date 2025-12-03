#!/bin/bash

# 전체 시스템 실행 스크립트
echo "🚀 IoT 컨트롤 시스템을 시작합니다..."

# 스크립트 디렉토리 설정 (절대 경로로 변환)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "📂 프로젝트 디렉토리: $SCRIPT_DIR"

# PID 저장 변수
CAM_PID=""
CPP_PID=""
FLASK_PID=""

# 정리 함수
cleanup() {
    echo ""
    echo "🛑 시스템 종료 중..."
    
    # Flask 서버 종료
    if [ ! -z "$FLASK_PID" ] && kill -0 $FLASK_PID 2>/dev/null; then
        echo "  → Flask 서버 종료 (PID: $FLASK_PID)"
        kill -TERM $FLASK_PID 2>/dev/null
        sleep 1
        kill -9 $FLASK_PID 2>/dev/null
    fi
    pkill -f "python.*app.py" 2>/dev/null || true
    
    # C++ 서버 종료
    if [ ! -z "$CPP_PID" ] && kill -0 $CPP_PID 2>/dev/null; then
        echo "  → C++ 서버 종료 (PID: $CPP_PID)"
        kill -TERM $CPP_PID 2>/dev/null
        sleep 1
        kill -9 $CPP_PID 2>/dev/null
    fi
    pkill -f "twiceproject" 2>/dev/null || true
    
    # 카메라 서버 종료
    if [ ! -z "$CAM_PID" ] && kill -0 $CAM_PID 2>/dev/null; then
        echo "  → 카메라 서버 종료 (PID: $CAM_PID)"
        kill -TERM $CAM_PID 2>/dev/null
        sleep 1
        kill -9 $CAM_PID 2>/dev/null
    fi
    pkill -f "cam.py" 2>/dev/null || true
    
    # 소켓 파일 정리
    echo "  → 소켓 파일 정리"
    rm -f /tmp/flaskToCPP.sock
    rm -f /tmp/aruco_socket
    
    echo "✅ 시스템 종료 완료!"
    exit 0
}

# 시그널 트랩 설정 (Ctrl+C, kill 등)
trap cleanup SIGINT SIGTERM SIGHUP EXIT

# 기존 프로세스 정리
echo "🛑 기존 프로세스 종료 중..."
pkill -f "python.*app.py" 2>/dev/null || true 
pkill -f "twiceproject" 2>/dev/null || true
pkill -f "cam.py" 2>/dev/null || true
sleep 2

# 기존 소켓 파일 삭제
echo "📁 기존 소켓 파일 정리..."
rm -f /tmp/flaskToCPP.sock
rm -f /tmp/aruco_socket

# 카메라 서버 실행
echo "📷 카메라 서버 시작 중..."
python3 "$SCRIPT_DIR/camPart/cam.py" &
CAM_PID=$!
echo "📋 카메라 서버 PID: $CAM_PID"
sleep 2

# 카메라 서버 상태 확인
if ! kill -0 $CAM_PID 2>/dev/null; then
    echo "❌ 카메라 서버 시작 실패!"
    exit 1
fi
echo "✅ 카메라 서버 시작 완료!"

# C++ 프로젝트 디렉토리로 이동하여 빌드
cd "$SCRIPT_DIR/CentralPart"
echo "🔨 C++ 프로젝트 빌드 중..."
rm -rf build
mkdir build
cd build
cmake -G Ninja ..
ninja

# C++ 서버 실행
if [ -f "twiceproject" ]; then
    echo "✅ C++ 서버 시작 중..."
    ./twiceproject &
    CPP_PID=$!
    echo "📋 C++ 서버 PID: $CPP_PID"
    
    # 잠시 대기 후 상태 확인
    sleep 3
    if kill -0 $CPP_PID 2>/dev/null; then
        echo "✅ C++ 서버 시작 완료!"
        
        # Flask 서버 시작
        echo "🌐 Flask 웹 서버 시작 중..."
        cd "$SCRIPT_DIR/webPart"
        
        # 가상환경 활성화 (있다면)
        VENV_PATH="$SCRIPT_DIR/../vkkw/bin/activate"
        if [ -f "$VENV_PATH" ]; then
            source "$VENV_PATH"
            echo "🐍 Python 가상환경 활성화됨"
        fi
        
        echo "🎯 Flask 서버를 시작합니다..."
        echo "📱 웹 인터페이스: http://<IP주소>:5000"
        echo ""
        echo "🛑 서버를 중지하려면 Ctrl+C를 누르세요"
        echo "   (C++ 서버 PID: $CPP_PID 도 함께 종료됩니다)"
        
        # 사용 가능한 포트 찾기
        PORT=5000
        while lsof -i :$PORT >/dev/null 2>&1; do
            echo "⚠️  포트 $PORT이 사용 중입니다. 다른 포트를 시도합니다..."
            PORT=$((PORT + 1))
        done
        
        echo "✅ 포트 $PORT을 사용합니다."
        echo "📱 웹 인터페이스: http://<IP주소>:$PORT"
        
        # Flask 서버 실행 (포그라운드)
        export FLASK_PORT=$PORT
        python app.py &
        FLASK_PID=$!
        echo "📋 Flask 서버 PID: $FLASK_PID"
        
        echo ""
        echo "✅ 전체 시스템 시작 완료!"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "📋 실행 중인 프로세스:"
        echo "  • 카메라 서버: $CAM_PID"
        echo "  • C++ 서버:    $CPP_PID"
        echo "  • Flask 서버:  $FLASK_PID"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "🛑 시스템 종료: Ctrl+C"
        echo ""
        
        # Flask 프로세스 모니터링
        wait $FLASK_PID
        
    else
        echo "❌ C++ 서버 시작 실패!"
        exit 1
    fi
else
    echo "❌ C++ 실행파일을 찾을 수 없습니다!"
    echo "📝 빌드를 먼저 수행하세요:"
    echo "   cmake -B build && cmake --build build"
    exit 1
fi