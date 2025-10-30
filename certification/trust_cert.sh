#!/bin/bash

# SSL 인증서를 시스템에 신뢰할 수 있는 인증서로 추가하는 스크립트

echo "🔒 SSL 인증서 신뢰 설정"
echo "=================================================="
echo ""

CERT_FILE="certs/cert.pem"

# 인증서 파일 확인
if [[ ! -f "$CERT_FILE" ]]; then
    echo "❌ 인증서 파일이 없습니다: $CERT_FILE"
    echo "💡 먼저 인증서를 생성하세요: python generate_cert.py"
    exit 1
fi

echo "📋 현재 OS를 확인하는 중..."
OS=$(uname -s)

if [[ "$OS" == "Linux" ]]; then
    echo "🐧 Linux 시스템 감지"
    echo ""
    echo "Debian/Ubuntu/Raspberry Pi OS 기준으로 진행합니다."
    echo ""
    
    # ca-certificates 패키지 확인
    if ! dpkg -l | grep -q ca-certificates; then
        echo "⚠️  ca-certificates 패키지가 설치되어 있지 않습니다."
        echo "설치하시겠습니까? (y/N): "
        read response
        if [[ "$response" =~ ^[Yy]$ ]]; then
            sudo apt-get update
            sudo apt-get install -y ca-certificates
        else
            exit 1
        fi
    fi
    
    # 인증서를 시스템 CA 저장소에 복사
    echo "📝 인증서를 시스템에 추가하는 중..."
    sudo cp "$CERT_FILE" /usr/local/share/ca-certificates/twiceproject-cert.crt
    
    # CA 인증서 업데이트
    echo "🔄 CA 인증서 데이터베이스 업데이트 중..."
    sudo update-ca-certificates
    
    echo ""
    echo "✅ 인증서가 시스템에 추가되었습니다!"
    echo ""
    echo "⚠️  브라우저별 추가 설정이 필요할 수 있습니다:"
    echo ""
    echo "📱 Chrome/Chromium:"
    echo "   1. chrome://settings/certificates"
    echo "   2. '기관' 탭 → '가져오기'"
    echo "   3. certs/cert.pem 선택"
    echo ""
    echo "📱 Firefox:"
    echo "   1. about:preferences#privacy"
    echo "   2. '인증서' → '인증서 보기'"
    echo "   3. '기관' 탭 → '가져오기'"
    echo "   4. certs/cert.pem 선택"
    echo ""
    
elif [[ "$OS" == "Darwin" ]]; then
    echo "🍎 macOS 시스템 감지"
    echo ""
    echo "📝 키체인에 인증서를 추가하는 중..."
    
    sudo security add-trusted-cert -d -r trustRoot \
        -k /Library/Keychains/System.keychain "$CERT_FILE"
    
    echo ""
    echo "✅ 인증서가 시스템 키체인에 추가되었습니다!"
    echo ""
    
else
    echo "⚠️  자동 설정이 지원되지 않는 OS입니다: $OS"
    echo ""
    echo "💡 수동으로 인증서를 신뢰하는 방법:"
    echo "   브라우저 설정에서 certs/cert.pem 파일을 가져오세요."
    exit 1
fi

echo "=================================================="
echo "🎉 완료!"
echo ""
echo "💡 다음 단계:"
echo "   1. 브라우저를 완전히 종료하고 다시 시작"
echo "   2. python app.py로 서버 시작"
echo "   3. https://localhost:5000 접속"
echo ""
echo "⚠️  모바일 기기:"
echo "   모바일에서는 여전히 경고가 표시될 수 있습니다."
echo "   각 기기에서 인증서를 수동으로 신뢰해야 합니다."
echo "=================================================="
