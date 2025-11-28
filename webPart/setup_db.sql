-- MariaDB 사용자 및 데이터베이스 설정 스크립트

-- 기존 사용자 삭제 (있다면)
DROP USER IF EXISTS 'iot_user'@'localhost';

-- 데이터베이스 생성
CREATE DATABASE IF NOT EXISTS iot_inventory
    DEFAULT CHARACTER SET utf8mb4
    DEFAULT COLLATE utf8mb4_unicode_ci;

-- 사용자 생성 및 권한 부여
CREATE USER 'iot_user'@'localhost' IDENTIFIED BY 'kkw';
GRANT ALL PRIVILEGES ON iot_inventory.* TO 'iot_user'@'localhost';
FLUSH PRIVILEGES;

-- 생성 확인
SELECT User, Host FROM mysql.user WHERE User = 'iot_user';
SHOW GRANTS FOR 'iot_user'@'localhost';

SELECT '✅ 사용자 및 데이터베이스 설정이 완료되었습니다!' AS message;
