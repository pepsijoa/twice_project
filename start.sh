#!/bin/bash

# 방향 제어 컨트롤러 시작 스크립트

echo "🎮 방향 제어 컨트롤러 시작"
echo "================================"
echo ""

# 가상환경 활성화 확인
if [[ "$VIRTUAL_ENV" == "" ]]; then
    echo "⚠️  가상환경이 활성화되지 않았습니다."
    echo "💡 다음 명령어로 활성화하세요:"
    echo "   source venv/bin/activate"
    echo ""
    read -p "자동으로 활성화하시겠습니까? (y/N): " response
    if [[ "$response" =~ ^[Yy]$ ]]; then
        source venv/bin/activate
        echo "✅ 가상환경이 활성화되었습니다."
    else
        echo "❌ 취소되었습니다."
        exit 1
    fi
fi

# Flask 설치 확인
if ! python -c "import flask" &> /dev/null; then
    echo "⚠️  Flask가 설치되어 있지 않습니다."
    read -p "Flask를 설치하시겠습니까? (y/N): " response
    if [[ "$response" =~ ^[Yy]$ ]]; then
        pip install flask
        echo "✅ Flask가 설치되었습니다."
    else
        echo "❌ Flask가 필요합니다. 설치 후 다시 시도하세요."
        exit 1
    fi
fi

# SSL 인증서 확인
if [[ ! -f "certification/certs/cert.pem" ]] || [[ ! -f "certification/certs/key.pem" ]]; then
    echo ""
    echo "⚠️  SSL 인증서가 없습니다."
    read -p "HTTPS용 인증서를 생성하시겠습니까? (y/N): " response
    if [[ "$response" =~ ^[Yy]$ ]]; then
        python certification/generate_cert.py
        if [[ $? -eq 0 ]]; then
            echo "✅ 인증서가 생성되었습니다."
        else
            echo "⚠️  인증서 생성에 실패했습니다. HTTP 모드로 실행됩니다."
        fi
    else
        echo "💡 HTTP 모드로 실행됩니다."
    fi
fi

# IP 주소 확인
echo ""
echo "================================"
echo "📡 네트워크 정보"
echo "================================"
IP=$(hostname -I | awk '{print $1}')
echo "🖥️  IP 주소: $IP"
echo ""

if [[ -f "certification/certs/cert.pem" ]] && [[ -f "certification/certs/key.pem" ]]; then
    echo "🔒 HTTPS 모드로 실행됩니다"
    echo "📱 접속 주소:"
    echo "   로컬: https://localhost:5000"
    echo "   원격: https://$IP:5000"
else
    echo "🌐 HTTP 모드로 실행됩니다"
    echo "📱 접속 주소:"
    echo "   로컬: http://localhost:5000"
    echo "   원격: http://$IP:5000"
fi

echo ""
echo "================================"
echo "🚀 서버를 시작합니다..."
echo "================================"
echo ""

# Flask 앱 실행
python app.py
