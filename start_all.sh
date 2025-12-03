#!/bin/bash

# 전체 시스템 실행 스크립트
echo "🚀 IoT 컨트롤 시스템을 시작합니다..."

# 기존 Flask 서버 종료
echo "🛑 기존 Flask 서버 종료 중..."
pkill -f "python.*app.py" 2>/dev/null || true
sleep 2

# 기존 C++ 서버 종료
echo "🛑 기존 C++ 서버 종료 중..."
pkill -f "twiceproject" 2>/dev/null || true
sleep 1

# 기존 소켓 파일 삭제
echo "📁 기존 소켓 파일 정리..."
rm -f /tmp/flaskToCPP.sock

# C++ 프로젝트 디렉토리로 이동하여 빌드
cd /home/kkw/iotclass/twice_project/CentralPart
echo "🔨 C++ 프로젝트 빌드 확인..."
cmake --build build

# C++ 서버 실행
if [ -f "build/twiceproject" ]; then
    echo "✅ C++ 서버 시작 중..."
    ./build/twiceproject &
    CPP_PID=$!
    echo "📋 C++ 서버 PID: $CPP_PID"
    
    # 잠시 대기 후 상태 확인
    sleep 3
    if kill -0 $CPP_PID 2>/dev/null; then
        echo "✅ C++ 서버 시작 완료!"
        
        # Flask 서버 시작
        echo "🌐 Flask 웹 서버 시작 중..."
        cd /home/kkw/iotclass/twice_project/webPart
        
        # 가상환경 활성화 (있다면)
        if [ -f "/home/kkw/iotclass/vkkw/bin/activate" ]; then
            source /home/kkw/iotclass/vkkw/bin/activate
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
        python app.py
        
        # Flask 서버가 종료되면 C++ 서버도 종료
        echo ""
        echo "🛑 Flask 서버가 종료되었습니다. C++ 서버도 종료합니다..."
        kill $CPP_PID 2>/dev/null
        echo "✅ 시스템 종료 완료!"
        
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