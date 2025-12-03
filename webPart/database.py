"""
MariaDB 데이터베이스 연결 및 관리 모듈
재고 관리 시스템을 위한 데이터베이스 유틸리티 함수 제공
"""

import pymysql
from pymysql import Error
from contextlib import contextmanager
import os
from datetime import datetime

# 데이터베이스 연결 설정
DB_CONFIG = {
    'host': os.environ.get('DB_HOST', 'localhost'),
    'user': os.environ.get('DB_USER', 'iot_user'),
    'password': os.environ.get('DB_PASSWORD', 'kkw'),
    'database': os.environ.get('DB_NAME', 'iot_inventory'),
    'charset': 'utf8mb4',
    'cursorclass': pymysql.cursors.DictCursor
}

@contextmanager
def get_db_connection():
    """
    데이터베이스 연결을 관리하는 컨텍스트 매니저
    자동으로 연결을 열고 닫음
    """
    connection = None
    try:
        connection = pymysql.connect(**DB_CONFIG)
        yield connection
    except Error as e:
        print(f"데이터베이스 연결 오류: {e}")
        raise
    finally:
        if connection:
            connection.close()

def init_database():
    """
    데이터베이스 초기화 (테이블이 없으면 생성)
    """
    try:
        with get_db_connection() as conn:
            with conn.cursor() as cursor:
                # 재고 테이블 생성
                cursor.execute("""
                    CREATE TABLE IF NOT EXISTS inventory (
                        id INT AUTO_INCREMENT PRIMARY KEY,
                        name VARCHAR(255) NOT NULL,
                        quantity INT NOT NULL DEFAULT 0,
                        location VARCHAR(100) NOT NULL,
                        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                        updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                        INDEX idx_name (name),
                        INDEX idx_location (location)
                    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
                """)
                conn.commit()
                print("✅ 데이터베이스 테이블 초기화 완료")
                return True
    except Error as e:
        print(f"❌ 데이터베이스 초기화 실패: {e}")
        return False

def get_all_inventory():
    """
    모든 재고 데이터 조회
    Returns: list of dict
    """
    try:
        with get_db_connection() as conn:
            with conn.cursor() as cursor:
                cursor.execute("""
                    SELECT id, name, quantity, location, 
                           DATE_FORMAT(updated_at, '%%Y-%%m-%%d') as updated_at
                    FROM inventory
                    ORDER BY location, name
                """)
                return cursor.fetchall()
    except Error as e:
        print(f"재고 조회 오류: {e}")
        return []

def get_inventory_by_id(item_id):
    """
    ID로 특정 재고 조회
    Args:
        item_id: 재고 ID
    Returns: dict or None
    """
    try:
        with get_db_connection() as conn:
            with conn.cursor() as cursor:
                cursor.execute("""
                    SELECT id, name, quantity, location, 
                           DATE_FORMAT(updated_at, '%%Y-%%m-%%d') as updated_at
                    FROM inventory
                    WHERE id = %s
                """, (item_id,))
                return cursor.fetchone()
    except Error as e:
        print(f"재고 조회 오류 (ID: {item_id}): {e}")
        return None

def search_inventory(keyword):
    """
    재고 검색 (이름으로 검색)
    Args:
        keyword: 검색 키워드
    Returns: list of dict
    """
    try:
        with get_db_connection() as conn:
            with conn.cursor() as cursor:
                cursor.execute("""
                    SELECT id, name, quantity, location, 
                           DATE_FORMAT(updated_at, '%%Y-%%m-%%d') as updated_at
                    FROM inventory
                    WHERE name LIKE %s OR location LIKE %s
                    ORDER BY location, name
                """, (f'%{keyword}%', f'%{keyword}%'))
                return cursor.fetchall()
    except Error as e:
        print(f"재고 검색 오류: {e}")
        return []

def add_inventory(name, quantity, location):
    """
    새로운 재고 추가
    Args:
        name: 제품 이름
        quantity: 수량
        location: 위치
    Returns: int (새로 생성된 ID) or None
    """
    try:
        with get_db_connection() as conn:
            with conn.cursor() as cursor:
                cursor.execute("""
                    INSERT INTO inventory (name, quantity, location)
                    VALUES (%s, %s, %s)
                """, (name, quantity, location))
                conn.commit()
                return cursor.lastrowid
    except Error as e:
        print(f"재고 추가 오류: {e}")
        return None

def update_inventory(item_id, name=None, quantity=None, location=None):
    """
    재고 정보 수정
    Args:
        item_id: 재고 ID
        name: 제품 이름 (선택)
        quantity: 수량 (선택)
        location: 위치 (선택)
    Returns: bool (성공 여부)
    """
    try:
        # 변경할 필드만 업데이트
        updates = []
        params = []
        
        if name is not None:
            updates.append("name = %s")
            params.append(name)
        if quantity is not None:
            updates.append("quantity = %s")
            params.append(quantity)
        if location is not None:
            updates.append("location = %s")
            params.append(location)
        
        if not updates:
            return False
        
        params.append(item_id)
        
        with get_db_connection() as conn:
            with conn.cursor() as cursor:
                sql = f"UPDATE inventory SET {', '.join(updates)} WHERE id = %s"
                cursor.execute(sql, params)
                conn.commit()
                # rowcount가 0이어도 업데이트 쿼리가 실행되었으면 성공 처리
                # (같은 값으로 업데이트하면 MySQL이 rowcount=0 반환)
                return cursor.rowcount >= 0
    except Error as e:
        print(f"재고 수정 오류 (ID: {item_id}): {e}")
        return False

def delete_inventory(item_id):
    """
    재고 삭제
    Args:
        item_id: 재고 ID
    Returns: bool (성공 여부)
    """
    try:
        with get_db_connection() as conn:
            with conn.cursor() as cursor:
                cursor.execute("DELETE FROM inventory WHERE id = %s", (item_id,))
                conn.commit()
                return cursor.rowcount > 0
    except Error as e:
        print(f"재고 삭제 오류 (ID: {item_id}): {e}")
        return False

def get_inventory_by_location(location):
    """
    특정 위치의 재고 조회
    Args:
        location: 위치
    Returns: list of dict
    """
    try:
        with get_db_connection() as conn:
            with conn.cursor() as cursor:
                cursor.execute("""
                    SELECT id, name, quantity, location, 
                           DATE_FORMAT(updated_at, '%%Y-%%m-%%d') as updated_at
                    FROM inventory
                    WHERE location = %s
                    ORDER BY name
                """, (location,))
                return cursor.fetchall()
    except Error as e:
        print(f"위치별 재고 조회 오류 (location: {location}): {e}")
        return []

def get_inventory_by_name(name):
    """
    제품 이름으로 재고 조회 (inventoryID와 매칭)
    Args:
        name: 제품 이름 (정수 문자열)
    Returns: dict or None
    """
    try:
        with get_db_connection() as conn:
            with conn.cursor() as cursor:
                cursor.execute("""
                    SELECT id, name, quantity, location, 
                           DATE_FORMAT(updated_at, '%%Y-%%m-%%d') as updated_at
                    FROM inventory
                    WHERE name = %s
                """, (name,))
                return cursor.fetchone()
    except Error as e:
        print(f"제품명 재고 조회 오류 (name: {name}): {e}")
        return None

def update_quantity(item_id, quantity_change):
    """
    재고 수량 증감
    Args:
        item_id: 재고 ID
        quantity_change: 변경할 수량 (양수: 증가, 음수: 감소)
    Returns: bool (성공 여부)
    """
    try:
        with get_db_connection() as conn:
            with conn.cursor() as cursor:
                cursor.execute("""
                    UPDATE inventory 
                    SET quantity = quantity + %s 
                    WHERE id = %s AND (quantity + %s) >= 0
                """, (quantity_change, item_id, quantity_change))
                conn.commit()
                return cursor.rowcount > 0
    except Error as e:
        print(f"재고 수량 변경 오류 (ID: {item_id}): {e}")
        return False

if __name__ == "__main__":
    # 테스트 및 초기화
    print("🔧 데이터베이스 초기화 중...")
    if init_database():
        print("✅ 데이터베이스가 성공적으로 초기화되었습니다.")
    else:
        print("❌ 데이터베이스 초기화에 실패했습니다.")
