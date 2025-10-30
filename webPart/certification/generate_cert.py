#!/usr/bin/env python3
"""
SSL 자체 서명 인증서 생성 스크립트
HTTPS로 Flask 서버를 실행하기 위한 인증서를 생성합니다.
"""

import os
import subprocess
import sys

def create_self_signed_cert():
    """자체 서명 SSL 인증서 생성"""
    
    # certs 디렉토리 생성
    cert_dir = os.path.join(os.path.dirname(__file__), 'certs')
    if not os.path.exists(cert_dir):
        os.makedirs(cert_dir)
        print(f"✅ '{cert_dir}' 디렉토리를 생성했습니다.")
    
    cert_file = os.path.join(cert_dir, 'cert.pem')
    key_file = os.path.join(cert_dir, 'key.pem')
    
    # 이미 인증서가 있는지 확인
    if os.path.exists(cert_file) and os.path.exists(key_file):
        print("⚠️  인증서가 이미 존재합니다.")
        response = input("새로 생성하시겠습니까? (y/N): ")
        if response.lower() != 'y':
            print("❌ 취소되었습니다.")
            return False
    
    print("\n🔐 자체 서명 SSL 인증서를 생성합니다...")
    print("=" * 50)
    
    # OpenSSL을 사용하여 인증서 생성
    cmd = [
        'openssl', 'req', '-x509', '-newkey', 'rsa:4096',
        '-keyout', key_file,
        '-out', cert_file,
        '-days', '365',
        '-nodes',
        '-subj', '/CN=localhost/O=IoT Controller/C=KR'
    ]
    
    try:
        subprocess.run(cmd, check=True, capture_output=True, text=True)
        print("✅ SSL 인증서가 성공적으로 생성되었습니다!")
        print(f"📄 인증서: {cert_file}")
        print(f"🔑 키 파일: {key_file}")
        print("\n" + "=" * 50)
        print("📱 이제 다음 명령어로 HTTPS 서버를 시작할 수 있습니다:")
        print("   python app.py")
        print("\n🌐 접속 주소:")
        print("   https://localhost:5000")
        print("   https://<라즈베리파이_IP>:5000")
        print("\n⚠️  주의사항:")
        print("   - 자체 서명 인증서이므로 브라우저에서 경고가 표시됩니다.")
        print("   - '고급' → '계속 진행'을 클릭하여 접속하세요.")
        print("   - 모바일에서도 인증서 경고를 수락해야 합니다.")
        print("=" * 50)
        return True
        
    except subprocess.CalledProcessError as e:
        print(f"❌ 오류 발생: {e}")
        print(f"   {e.stderr}")
        return False
    except FileNotFoundError:
        print("❌ OpenSSL이 설치되어 있지 않습니다.")
        print("💡 설치 방법:")
        print("   sudo apt-get update")
        print("   sudo apt-get install openssl")
        return False

def main():
    print("=" * 50)
    print("🔒 SSL 인증서 생성 도구")
    print("=" * 50)
    print()
    
    if create_self_signed_cert():
        print("\n✅ 완료!")
        return 0
    else:
        print("\n❌ 실패!")
        return 1

if __name__ == '__main__':
    sys.exit(main())
