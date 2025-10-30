const CACHE_NAME = 'direction-controller-v1';
const urlsToCache = [
  '/',
  '/static/css/style.css',
  '/static/js/script.js',
  '/static/images/icon-192.png',
  '/static/images/icon-512.png'
];

// Service Worker 설치
self.addEventListener('install', event => {
  console.log('🔧 Service Worker 설치 중...');
  event.waitUntil(
    caches.open(CACHE_NAME)
      .then(cache => {
        console.log('📦 캐시 열기 성공');
        return cache.addAll(urlsToCache);
      })
      .catch(error => {
        console.error('❌ 캐시 추가 실패:', error);
      })
  );
});

// Service Worker 활성화
self.addEventListener('activate', event => {
  console.log('✅ Service Worker 활성화');
  event.waitUntil(
    caches.keys().then(cacheNames => {
      return Promise.all(
        cacheNames.map(cacheName => {
          if (cacheName !== CACHE_NAME) {
            console.log('🗑️ 오래된 캐시 삭제:', cacheName);
            return caches.delete(cacheName);
          }
        })
      );
    })
  );
});

// 네트워크 요청 가로채기
self.addEventListener('fetch', event => {
  event.respondWith(
    caches.match(event.request)
      .then(response => {
        // 캐시에 있으면 캐시에서 반환, 없으면 네트워크 요청
        if (response) {
          return response;
        }
        return fetch(event.request).then(
          response => {
            // 유효한 응답이 아니면 그냥 반환
            if(!response || response.status !== 200 || response.type !== 'basic') {
              return response;
            }
            
            // 응답을 복제하여 캐시에 저장
            const responseToCache = response.clone();
            caches.open(CACHE_NAME)
              .then(cache => {
                cache.put(event.request, responseToCache);
              });
            
            return response;
          }
        );
      })
      .catch(error => {
        console.error('❌ Fetch 실패:', error);
      })
  );
});
