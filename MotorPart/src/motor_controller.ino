/*
 * FreeRTOS: UART String Command 
 * - SerialTask (Priority 1)
 * - MotorTask (Priority 2)
 * - Motor_ESTOP (Priority 3)
 * 두 태스크가 Queue를 통해 통신합니다.
 */ 

#include <Arduino_FreeRTOS.h>
#include <queue.h> // Queue API 헤더

//--- 1. 하드웨어/핀 정의 ---
// (모터 핀들을 여기에 정의하세요)
// #define MOTOR_A_IN1 9
// #define MOTOR_A_IN2 10
// ...

#define MOTOR_A_IN1 9
#define MOTOR_A_IN2 10

#define MOTOR_B_IN1 11
#define MOTOR_B_IN2 12 

// 초음파 센서 
#define TRIG_PIN 7  
#define ECHO_PIN 8
#define STOP_DISTANCE_CM 10

//--- 2. Queue 핸들 정의 ---
QueueHandle_t xMotorQueue;

//--- 3. Queue로 보낼 데이터 타입 정의 ---
typedef enum {
  CMD_STOP,
  CMD_UP,
  CMD_DOWN,
  CMD_LEFT,
  CMD_RIGHT,
  CMD_INVALID
} MotorCommand_t; // MotorCommand_t 라는 새로운 타입을 만듦

// (include 및 define 밑에 추가)

/**
 * @brief MotorCommand_t enum 값을 문자열로 변환하는 헬퍼 함수
 * @param cmd 명령어 enum 값
 * @return const char* (문자열 포인터)
 */
const char* getCommandString(MotorCommand_t cmd) {
  switch (cmd) {
    case CMD_STOP:   return "STOP";
    case CMD_UP:     return "UP";
    case CMD_DOWN:   return "DOWN";
    case CMD_LEFT:   return "LEFT";
    case CMD_RIGHT:  return "RIGHT";
    default:         return "INVALID";
  }
}

//==================================================
// setup() : C언어의 main() 함수 역할
//==================================================
void setup() {
  Serial.begin(115200);
  while (!Serial) { ; } // 시리얼 포트 대기

  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_IN1, OUTPUT);
  pinMode(MOTOR_B_IN2, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  //--- 1. 큐 생성 ---
  xMotorQueue = xQueueCreate(1, sizeof(MotorCommand_t));

  if (xMotorQueue == NULL) {
    Serial.println("큐 생성 실패!");
    while(1); // 시스템 정지
  }

  //--- 2. 태스크 생성 ---
  xTaskCreate(
    prvSerialTask,    // 태스크 함수 포인터
    "SerialTask",     // 태스크 이름
    128,              // 스택 크기 (word 단위)
    NULL,             // 태스크 파라미터
    1,                // 우선순위 (낮음)
    NULL);            // 태스크 핸들 (안 씀)

  xTaskCreate(
    prvMotorTask,     // 태스크 함수 포인터
    "MotorTask",      // 태스크 이름
    128,              // 스택 크기
    NULL,             // 태스크 파라미터
    2,                // 우선순위 (높음)
    NULL);  // 태스크 핸들
  
  xTaskCreate(
    prvMotor_ESTOP,
    "US_Sensor",
    128,
    NULL,
    3,
    NULL);
  
  //--- 3. 스케줄러 시작 ---
  // 이 시점부터 setup()은 끝나고 제어권은 RTOS로 넘어감
  vTaskStartScheduler();
}

//==================================================
// loop() : 절대로 실행되지 않음
//==================================================
void loop() {
  // 비워둠
}

void prvSerialTask(void *pvParameters) {
  (void) pvParameters;

  uint8_t rx_byte;
  MotorCommand_t cmd_to_send = CMD_INVALID; 

  // C언어 스타일의 무한 루프
  for (;;) {
    
    // 시리얼 포트에 읽을 데이터가 있는지 확인 (논블로킹)
    if (Serial.available() > 0) {
      rx_byte = (uint8_t)Serial.read();
      
      uint8_t orientation = rx_byte & 0b00001111;

      switch(orientation) {
        case 0x01:
          cmd_to_send = CMD_RIGHT;
          break;
        case 0x02:
          cmd_to_send = CMD_LEFT;
          break;
        case 0x04:
          cmd_to_send = CMD_DOWN;
          break;
        case 0x08:
          cmd_to_send = CMD_UP;
          break;
      }

      Serial.print("Received (Byte): 0x");
      Serial.println(rx_byte, HEX);

      xQueueOverwrite(xMotorQueue, &cmd_to_send);

    // 이 태스크를 잠시(10ms) 재워서 다른 태스크(MotorTask)가 실행될 시간을 줌
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void prvMotorTask(void *pvParameters) {
  (void) pvParameters;
  MotorCommand_t received_cmd;

  // C언어 스타일의 무한 루프
  for (;;) {
    //--- 큐에서 명령이 올 때까지 무한정 대기 (Blocked 상태) ---
    // 큐에 데이터가 들어오면 이 태스크는 즉시 'Ready' 상태가 됨
    if (xQueueReceive(xMotorQueue, &received_cmd, portMAX_DELAY) == pdPASS) {
      
      Serial.println(getCommandString(received_cmd)); // 헬퍼 함수 사용
      switch (received_cmd) {
        case CMD_UP:
          Motor_UP();
          Serial.println("===> 모터: 위로 이동");
          break;
        case CMD_DOWN:
          Motor_DOWN();
          Serial.println("===> 모터: 아래로 이동");
          break;
        case CMD_LEFT:
          Motor_LEFT();
          Serial.println("===> 모터: 왼쪽으로 이동");
          break;
        case CMD_RIGHT:
          Motor_RIGHT();
          Serial.println("===> 모터: 오른쪽으로 이동");
          break;
        case CMD_STOP:
          Motor_STOP();
          Serial.println("===> 모터: 정지");
          break;
        case CMD_INVALID:
        default:
          Serial.println("===> Error");
          // 추후 protocol 작성 후 에러 값 표출 
          break;
      }
    }
  }
}

void prvMotor_ESTOP(void *pvParameters) {
  (void) pvParameters;

  long duration;
  int distance;
  MotorCommand_t estop_cmd = CMD_STOP; // E-STOP은 항상 STOP 명령만 보냄
  
  // 100ms마다 이 태스크를 실행
  const TickType_t xFrequency = 100 / portTICK_PERIOD_MS;

  // C언어 스타일의 무한 루프
  for (;;) {
    // --- 1. Python 코드의 초음파 트리거 부분 ---
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10); // 10us 펄스
    digitalWrite(TRIG_PIN, LOW);

    // --- 2. Echo 수신 (pulseIn 사용) ---
    // ECHO 핀이 HIGH가 될 때까지 기다렸다가, 
    // LOW가 될 때까지의 시간을 (us) 측정
    // 30000us (30ms) 타임아웃: 너무 멀리 있거나 장애물이 없으면 영원히 기다리지 않음
    duration = pulseIn(ECHO_PIN, HIGH, 30000);

    // --- 3. 거리 계산 (cm) ---
    // (duration / 2) * 0.0343 (소리 속도)  ==> duration / 58.3
    distance = duration / 58;

    // --- 4. 위험 감지 및 명령 전송 ---
    // (distance > 0 은 pulseIn이 타임아웃되지 않았다는 의미)
    if (distance > 0 && distance < STOP_DISTANCE_CM) {
      
      // 위험!!
      // Serial.println("!!! E-STOP TRIGGERED !!!"); // (디버깅용)
      
      xQueueOverwrite(xMotorQueue, &estop_cmd);
    }
    vTaskDelay(xFrequency);
  }
}

void Motor_UP() {
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);

  digitalWrite(MOTOR_B_IN1, HIGH);
  digitalWrite(MOTOR_B_IN2, LOW);
}

void Motor_STOP() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, LOW);

  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, LOW);
}

void Motor_LEFT() {
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);

  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, HIGH);
}

void Motor_RIGHT() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);

  digitalWrite(MOTOR_B_IN1, HIGH);
  digitalWrite(MOTOR_B_IN2, LOW);
}

void Motor_DOWN() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);

  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, HIGH);
}
