#!/usr/bin/env python3
"""
개선된 SSL 인증서 생성 스크립트
SAN (Subject Alternative Name)을 포함하여 더 안전한 인증서를 생성합니다.
"""

import os
import subprocess
import sys
import socket

def get_local_ip():
    """로컬 IP 주소 가져오기"""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except:
        return "127.0.0.1"

def create_openssl_config(ip_address, hostname):
    """OpenSSL 설정 파일 생성"""
    config_content = f"""[req]
default_bits = 2048
prompt = no
default_md = sha256
x509_extensions = v3_req
distinguished_name = dn

[dn]
C = KR
ST = Seoul
L = Seoul
O = IoT Controller
OU = Development
CN = {hostname}

[v3_req]
subjectAltName = @alt_names

[alt_names]
DNS.1 = localhost
DNS.2 = {hostname}
DNS.3 = {hostname}.local
IP.1 = 127.0.0.1
IP.2 = {ip_address}
"""
    return config_content

def create_advanced_cert():
    """SAN을 포함한 고급 인증서 생성"""
    
    # certs 디렉토리 생성
    cert_dir = os.path.join(os.path.dirname(__file__), 'certs')
    if not os.path.exists(cert_dir):
        os.makedirs(cert_dir)
        print(f"✅ '{cert_dir}' 디렉토리를 생성했습니다.")
    
    cert_file = os.path.join(cert_dir, 'cert.pem')
    key_file = os.path.join(cert_dir, 'key.pem')
    config_file = os.path.join(cert_dir, 'openssl.cnf')
    
    # 이미 인증서가 있는지 확인
    if os.path.exists(cert_file) and os.path.exists(key_file):
        print("⚠️  인증서가 이미 존재합니다.")
        response = input("새로 생성하시겠습니까? (y/N): ")
        if response.lower() != 'y':
            print("❌ 취소되었습니다.")
            return False
    
    # 시스템 정보 가져오기
    hostname = socket.gethostname()
    ip_address = get_local_ip()
    
    print("\n🔐 고급 SSL 인증서를 생성합니다...")
    print("=" * 50)
    print(f"🖥️  호스트명: {hostname}")
    print(f"🌐 IP 주소: {ip_address}")
    print("=" * 50)
    
    # OpenSSL 설정 파일 생성
    config_content = create_openssl_config(ip_address, hostname)
    with open(config_file, 'w') as f:
        f.write(config_content)
    print(f"✅ OpenSSL 설정 파일 생성: {config_file}")
    
    # OpenSSL을 사용하여 인증서 생성
    cmd = [
        'openssl', 'req', '-x509', '-newkey', 'rsa:2048',
        '-keyout', key_file,
        '-out', cert_file,
        '-days', '365',
        '-nodes',
        '-config', config_file
    ]
    
    try:
        subprocess.run(cmd, check=True, capture_output=True, text=True)
        print("✅ SSL 인증서가 성공적으로 생성되었습니다!")
        print(f"📄 인증서: {cert_file}")
        print(f"🔑 키 파일: {key_file}")
        print(f"⚙️  설정 파일: {config_file}")
        print("\n" + "=" * 50)
        print("🌐 접속 주소 (경고 최소화):")
        print(f"   https://localhost:5000")
        print(f"   https://{hostname}.local:5000")
        print(f"   https://{ip_address}:5000")
        print("\n💡 경고를 완전히 제거하려면:")
        print("   sudo ./trust_cert.sh")
        print("\n⚠️  주의사항:")
        print("   - 자체 서명 인증서이므로 브라우저 경고가 표시될 수 있습니다.")
        print("   - trust_cert.sh를 실행하여 시스템에 인증서를 신뢰하도록 추가하세요.")
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
    print("🔒 고급 SSL 인증서 생성 도구 (SAN 지원)")
    print("=" * 50)
    print()
    
    if create_advanced_cert():
        print("\n✅ 완료!")
        return 0
    else:
        print("\n❌ 실패!")
        return 1

if __name__ == '__main__':
    sys.exit(main())
