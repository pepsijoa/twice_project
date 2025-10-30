from PIL import Image, ImageDraw, ImageFont
import os

# 이미지 생성 함수
def create_icon(size, filename):
    # 그라데이션 배경 생성 (보라색 계열)
    img = Image.new('RGB', (size, size), color='#667eea')
    draw = ImageDraw.Draw(img)
    
    # 원형 배경
    margin = size // 10
    draw.ellipse([margin, margin, size-margin, size-margin], 
                 fill='#764ba2', outline='white', width=size//30)
    
    # 화살표 그리기 (간단한 십자가 형태)
    center = size // 2
    arrow_size = size // 4
    line_width = size // 15
    
    # 상하좌우 화살표
    # 위
    draw.polygon([
        (center, center - arrow_size),
        (center - arrow_size//2, center - arrow_size//2),
        (center + arrow_size//2, center - arrow_size//2)
    ], fill='white')
    
    # 아래
    draw.polygon([
        (center, center + arrow_size),
        (center - arrow_size//2, center + arrow_size//2),
        (center + arrow_size//2, center + arrow_size//2)
    ], fill='white')
    
    # 왼쪽
    draw.polygon([
        (center - arrow_size, center),
        (center - arrow_size//2, center - arrow_size//2),
        (center - arrow_size//2, center + arrow_size//2)
    ], fill='white')
    
    # 오른쪽
    draw.polygon([
        (center + arrow_size, center),
        (center + arrow_size//2, center - arrow_size//2),
        (center + arrow_size//2, center + arrow_size//2)
    ], fill='white')
    
    # 중앙 원
    circle_size = size // 8
    draw.ellipse([center - circle_size, center - circle_size, 
                  center + circle_size, center + circle_size], 
                 fill='white')
    
    # 저장
    img.save(filename, 'PNG')
    print(f'✅ {filename} 생성 완료')

# 스크립트 실행 경로
script_dir = os.path.dirname(os.path.abspath(__file__))
images_dir = os.path.join(script_dir, 'images')

# 이미지 생성
create_icon(192, os.path.join(images_dir, 'icon-192.png'))
create_icon(512, os.path.join(images_dir, 'icon-512.png'))

print('🎉 모든 아이콘 생성 완료!')
