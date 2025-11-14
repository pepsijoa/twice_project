/*
 * FreeRTOS: UART String Command 
 * - SerialTask (Priority 1)
 * - MotorTask (Priority 2)
 * - Motor_ESTOP (Priority 3)
 * 두 태스크가 Queue를 통해 통신합니다.
 */ 

#include <Arduino_FreeRTOS.h>
#include <queue.h> // Queue API 헤더

#define MOTOR_A_IN1 9
#define MOTOR_A_IN2 10

#define MOTOR_B_IN1 11
#define MOTOR_B_IN2 12 

#define ENABLE_A 3
#define ENABLE_B 5
// 초음파 센서 
#define TRIG_PIN 7  
#define ECHO_PIN 8
#define STOP_DISTANCE_CM 10

//--- 2. Queue 핸들 정의 ---
QueueHandle_t xMotorQueue;

TaskHandle_t Motor_Control;
TaskHandle_t Motor_ESTOP;

//--- 3. Queue로 보낼 데이터 타입 정의 ---
typedef enum {
  CMD_STOP,
  CMD_UP,
  CMD_DOWN,
  CMD_LEFT,
  CMD_RIGHT,
  CMD_INVALID
} MotorCommand_t; // MotorCommand_t 라는 새로운 타입을 만듦

volatile MotorCommand_t g_currentMotorState = CMD_STOP;
volatile bool g_isObstacleDetected = false; // E-STOP 상태 플래그

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

void setup() {
  Serial.begin(115200);
  
  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_IN1, OUTPUT);
  pinMode(MOTOR_B_IN2, OUTPUT);

  pinMode(ENABLE_A, OUTPUT);
  pinMode(ENABLE_B, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  //--- 1. 큐 생성 ---
  xMotorQueue = xQueueCreate(1, sizeof(MotorCommand_t));

  if (xMotorQueue == NULL) {
    //Serial.println("큐 생성 실패!");
    while(1); // 시스템 정지
  }

  //--- 2. 태스크 생성 ---
  xTaskCreate(
    prvSerialTask,    // 태스크 함수 포인터
    "SerialTask & RX",     // 태스크 이름
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
    &Motor_Control);  // 태스크 핸들
  
  xTaskCreate(
    prvMotor_ESTOP,
    "Sensor & TX",
    128,
    NULL,
    3,
    &Motor_ESTOP);
  
  vTaskStartScheduler();
}

void loop() {}

void prvSerialTask(void *pvParameters) {
  (void) pvParameters;

  uint8_t rx_byte;
  MotorCommand_t cmd_to_send = CMD_INVALID; 

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

      xQueueOverwrite(xMotorQueue, &cmd_to_send);
      
      // 명령 수신 직후 즉시 응답 전송
      uint8_t response_byte;
      if (g_isObstacleDetected) {
        response_byte = 0b01101110; // 'n' - 장애물 감지됨
      } else {
        response_byte = 0b01111001; // 'y' - 정상
      }
      Serial.write(response_byte);
    }
    // 이 태스크를 잠시(100ms) 재워서 다른 태스크(MotorTask)가 실행될 시간을 줌
    vTaskDelay(100 / portTICK_PERIOD_MS);
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
      
      //Serial.println(getCommandString(received_cmd)); // 헬퍼 함수 사용
      switch (received_cmd) {
        case CMD_UP:
          Motor_UP();
          break;
        case CMD_DOWN:
          Motor_DOWN();
          break;
        case CMD_LEFT:
          Motor_LEFT();
          break;
        case CMD_RIGHT:
          Motor_RIGHT();
          break;
        case CMD_STOP:
          Motor_STOP();
          break;
        case CMD_INVALID:
        default:
          // 추후 protocol 작성 후 에러 값 표출 
          break;
      }
    }
  }
}

void prvMotor_ESTOP(void *pvParameters) {
  (void) pvParameters;

  long duration;
  int distance =0;
  MotorCommand_t estop_cmd = CMD_STOP; // E-STOP은 항상 STOP 명령만 보냄
  
  const TickType_t xFrequency = 100 / portTICK_PERIOD_MS;

  uint8_t tx_byte = 0b00000001;

  for (;;) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10); 
    digitalWrite(TRIG_PIN, LOW);

    duration = pulseIn(ECHO_PIN, HIGH, 5000);

    distance = duration / 58;

    if (distance > 0 && distance < STOP_DISTANCE_CM) {
      // Serial.println("!!! E-STOP TRIGGERED !!!"); // (디버깅용)
      g_isObstacleDetected = true;  // 전역 플래그 설정
      xQueueOverwrite(xMotorQueue, &estop_cmd);
      tx_byte = 0b01101110;  // 'n'
    }
    else {
      g_isObstacleDetected = false;  // 전역 플래그 해제
      tx_byte = 0b01111001;  // 'y'
    }
    // prvMotor_ESTOP은 더 이상 Serial.write()를 하지 않음
    // 응답은 prvSerialTask에서 처리
    // Serial.write(tx_byte);  // 주석 처리
    vTaskDelay(xFrequency);
  }
}

void motor_speed(int spd)  
{  
  analogWrite(ENABLE_A,spd);  
  analogWrite(ENABLE_B,spd);  
}  

void Motor_UP() {
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);

  digitalWrite(MOTOR_B_IN1, HIGH);
  digitalWrite(MOTOR_B_IN2, LOW);
  motor_speed(100);
}

void Motor_STOP() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, LOW);

  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, LOW);
  motor_speed(150);
}

void Motor_LEFT() {
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);

  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, HIGH);
  motor_speed(150);
}

void Motor_RIGHT() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);

  digitalWrite(MOTOR_B_IN1, HIGH);
  digitalWrite(MOTOR_B_IN2, LOW);
  motor_speed(150);
}

void Motor_DOWN() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);

  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, HIGH);
  motor_speed(150);
}