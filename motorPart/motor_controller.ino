/*
 * FreeRTOS: UART String Command 
 * - SerialTask (Priority 1)
 * - MotorTask (Priority 2)
 * - Motor_ESTOP (Priority 3)
 * 두 태스크가 Queue를 통해 통신합니다.
 */ 

#include <Arduino_FreeRTOS.h>
#include <queue.h> // Queue API 헤더
#include <PinChangeInterrupt.h>
#include <Wire.h>
#include <MPU6050_light.h>
#include <PID_v1.h>

#define MOTOR_A_IN1 9
#define MOTOR_A_IN2 10

#define MOTOR_B_IN1 11
#define MOTOR_B_IN2 12 

#define ENABLE_A 3
#define ENABLE_B 5
// 초음파 센서 
#define TRIG_PIN 7  
#define ECHO_PIN 8

#define ENCODER_A_INT A0
#define ENCODER_B_INT A1

#define IMU_INT 2 

const int STOP_DISTANCE_CM = 10;
// MOTOR HARDWARE SPEC
const float WHEEL_DIAMETER_CM = 6.5f;
const float COUNTS_PER_REV = 20.0;

#define DIST_PER_PULSE (PI * WHEEL_DIAMETER_CM / COUNTS_PER_REV)

volatile long g_ENCODER_LEFT_COUNT = 0l; 
volatile long g_ENCODER_RIGHT_COUNT = 0l;

volatile float g_robot_x = 0.0;
volatile float g_robot_y = 0.0;
volatile float g_robot_theta = 0.0;

MPU6050 mpu(Wire);

typedef struct _FloatPacket {
  float value;
  uint8_t bytes[4];
} FloatPacket_t;

// PID 
double P_Input, P_Output, P_Setpoint;
// Kp, Ki, Kd는 튜닝이 필요하지만 일단 국민값으로 시작
double Kp = 2.0, Ki = 0.0, Kd = 0.1; 
PID myPID(&P_Input, &P_Output, &P_Setpoint, Kp, Ki, Kd, DIRECT);

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

void ISR_Encoder_A(){
  g_ENCODER_LEFT_COUNT++;  
}

void ISR_Encoder_B(){
  g_ENCODER_RIGHT_COUNT++;
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

// (전역 변수 선언부는 동일하게 유지)
// 주의: g_robot_theta는 라디안 단위라면 도(Degree)로 변환 필요

void rotate_sequence(float relative_target_angle) {
    // 1. 목표 설정 (상대 각도 적용)
    // 현재 각도를 기준으로 목표를 잡습니다.
    // (주의: g_robot_theta가 라디안이면 * 180/PI 해서 도단위로 변환 후 사용)
    float current_angle = g_robot_theta * 180.0 / PI; 
    float target_angle = current_angle + relative_target_angle;

    // PID 설정
    P_Setpoint = target_angle;
    myPID.SetMode(AUTOMATIC);
    myPID.SetOutputLimits(-150, 150); // PWM 범위 (너무 빠르면 제어 안됨)
    
    float angle_tolerance = 2.0; // 3도 오차 허용

    // 2. 메인 제어 루프
    while (fabs(target_angle - current_angle) > angle_tolerance) {
        
        // --- [중요] 큐에 새로운 긴급 명령이 왔는지 확인 (Peek) ---
        // 큐에서 데이터를 꺼내지는 않고(Peek), 내용만 봅니다.
        MotorCommand_t new_cmd;
        if (xQueuePeek(xMotorQueue, &new_cmd, 0) == pdPASS) {
            // 만약 STOP 명령이나 다른 방향 명령이 들어왔다면?
            if (new_cmd != CMD_INVALID) {
                Motor_STOP(); // 즉시 정지
                return;       // 회전 함수 강제 종료! (MotorTask 루프로 복귀)
            }
        }

        // A. 입력 갱신 (MPU 직접 호출 X -> 전역 변수 사용 O)
        // SensorTask가 g_robot_theta를 계속 업데이트해주고 있다고 가정
        current_angle = g_robot_theta * 180.0 / PI; 
        P_Input = current_angle;
        
        // B. PID 계산
        myPID.Compute(); 
        
        int pwm_val = abs(P_Output);
        if (pwm_val < 80) pwm_val = 80; // 모터가 돌 수 있는 최소 전압 보장

        // C. 모터 제어 (제자리 회전)
        // PID Output이 양수면 우회전, 음수면 좌회전이라고 가정
        if (P_Output > 0) {
            // 오른쪽으로 돌려면: 왼쪽 바퀴 전진, 오른쪽 바퀴 후진
            // (Motor_RIGHT 함수가 제자리 회전용으로 구현되어 있어야 함)
            // Motor_RIGHT(abs(Output)); 
            
            digitalWrite(MOTOR_A_IN1, HIGH); digitalWrite(MOTOR_A_IN2, LOW); // 왼쪽 전진
            digitalWrite(MOTOR_B_IN1, LOW);  digitalWrite(MOTOR_B_IN2, HIGH); // 오른쪽 후진
            analogWrite(ENABLE_A, pwm_val);
            analogWrite(ENABLE_B, pwm_val);

        } else {
            // 반시계 방향
            digitalWrite(MOTOR_A_IN1, LOW);  digitalWrite(MOTOR_A_IN2, HIGH); // 왼쪽 후진
            digitalWrite(MOTOR_B_IN1, HIGH); digitalWrite(MOTOR_B_IN2, LOW); // 오른쪽 전진
            analogWrite(ENABLE_A, pwm_val);
            analogWrite(ENABLE_B, pwm_val);
        }

        // D. 태스크 지연 (다른 태스크에게 CPU 양보)
        vTaskDelay(20 / portTICK_PERIOD_MS); 
    }
    
    Motor_STOP(); // 회전 완료 후 정지
}

// 목표 절대 각도를 받아서, 회전 후 직진하는 함수
void move_to_absolute_direction(float target_absolute_angle) {
    
    // 1. 현재 로봇의 각도 (IMU 값, 도 단위)
    float current_angle = g_robot_theta * 180.0 / PI;

    // 2. 회전해야 할 양 계산 (최단 경로 회전)
    // 예: 현재 10도, 목표 -90도 -> -100도 회전
    float diff = target_absolute_angle - current_angle;

    // 각도 정규화 (-180 ~ 180도로 맞추기)
    // 예: 270도 회전하는 것보다 반대로 -90도 회전하는 게 빠름
    while (diff > 180) diff -= 360;
    while (diff < -180) diff += 360;

    // 3. 오차가 크면 회전부터 실행
    // (5도 이상 차이 나면 회전 시퀀스 진입)
    if (abs(diff) > 4.0) {
        // diff만큼 상대 회전 (이전에 만든 rotate_sequence 재활용)
        rotate_sequence(diff);
    }

    // 4. 방향이 맞으면 직진
    Motor_UP(); 
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

  pinMode(ENCODER_A_INT, INPUT_PULLUP);
  pinMode(ENCODER_B_INT, INPUT_PULLUP);

  pinMode(IMU_INT, INPUT_PULLUP);

  attachPCINT(digitalPinToPCINT(ENCODER_A_INT), ISR_Encoder_A, CHANGE);
  attachPCINT(digitalPinToPCINT(ENCODER_B_INT), ISR_Encoder_B, CHANGE);

  //--- 1. 큐 생성 ---
  xMotorQueue = xQueueCreate(1, sizeof(MotorCommand_t));

  if (xMotorQueue == NULL) {
    //Serial.println("큐 생성 실패!");
    while(1); // 시스템 정지
  }

  //--- 2. 태스크 생성 ---
  xTaskCreate(
    prvRX,    // 태스크 함수 포인터
    "RX",     // 태스크 이름
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
    prvSensorandTX,
    "Sensor & TX",
    128,
    NULL,
    3,
    &Motor_ESTOP);
  
  vTaskStartScheduler();
}



void loop() {}

void prvRX(void *pvParameters) {
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
      switch (received_cmd) {
        case CMD_UP:
          move_to_absolute_direction(0.0);
          break;
        case CMD_DOWN:
          move_to_absolute_direction(180.0);
          break;
        case CMD_LEFT:
          move_to_absolute_direction(-90.0);
          break;
        case CMD_RIGHT:
          move_to_absolute_direction(90.0);
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

void prvSensorandTX(void *pvParameters) {
  (void) pvParameters;
  Wire.begin();
  byte status = mpu.begin();
  if(status == 0){
    delay(1000);
    mpu.calcOffsets(); // 초점 잡기
  }

  long prev_Encoder_A = 0;
  long prev_Encoder_B = 0;
  float prev_theta = 0; 

  FloatPacket_t px, py, ptheta;
  uint8_t tx_byte = 0; 

  // portTICK_PERIOD_MS = 주기 / ms  50ms 
  const TickType_t xFrequency = 50 / portTICK_PERIOD_MS; 
  TickType_t xLastWakeTime = xTaskGetTickCount();

  MotorCommand_t estop_cmd = CMD_STOP; // E-STOP은 항상 STOP 명령만 보냄

  for (;;) {
    unsigned long startTime = micros(); // 1. 시작 시간 기록
    mpu.update();

    long curr_ENCODER_A; long curr_ENCODER_B;
    taskENTER_CRITICAL();
    curr_ENCODER_A = g_ENCODER_LEFT_COUNT;
    curr_ENCODER_B = g_ENCODER_RIGHT_COUNT;
    taskEXIT_CRITICAL();

    long diff_ENCODER_A = curr_ENCODER_A - prev_Encoder_A;
    long diff_ENCODER_B = curr_ENCODER_B - prev_Encoder_B;

    prev_Encoder_A = curr_ENCODER_A;
    prev_Encoder_B = curr_ENCODER_B;
    prev_theta = g_robot_theta;

    // 후진 판단

    float dir_factor = 1.0;
    if (g_currentMotorState == CMD_DOWN) dir_factor = -1.0;

    float distance_A = diff_ENCODER_A * DIST_PER_PULSE * dir_factor;
    float distance_B = diff_ENCODER_B * DIST_PER_PULSE * dir_factor;
    float dist_center = (distance_A + distance_B) / 2.0;

    // Rotation 각도

    float current_angle_deg = mpu.getAngleZ();
    g_robot_theta = current_angle_deg * PI / 180;
    float avg_theta = (prev_theta + g_robot_theta) / 2.0;
    // 2-5. 좌표 적분 (Dead Reckoning)
    // x' = x + d * cos(theta)
    // y' = y + d * sin(theta)
    // (만약 제자리 회전 중이라면 dist_center가 거의 0이므로 위치 안 변함 -> 정확!)
    g_robot_x += dist_center * cos(avg_theta);
    g_robot_y += dist_center * sin(avg_theta);

    digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10); 
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 5000);
    int distance = duration / 58;

    bool isEmergency = distance > 0 && distance < STOP_DISTANCE_CM;
    if (isEmergency) {
      g_isObstacleDetected = true;  // 전역 플래그 설정
      xQueueOverwrite(xMotorQueue, &estop_cmd);
    }
    else {
      g_isObstacleDetected = false;  // 전역 플래그 해제
    }
    px.value = g_robot_x;
    py.value = g_robot_y;
    ptheta.value = g_robot_theta;

    if (isEmergency) tx_byte = 0x02;
    else tx_byte = 0x01;

    Serial.write(0xAA);        // 설정을 안하면 1 byte 
    Serial.write(0xBB);
    Serial.write(px.bytes, sizeof(px.bytes)); // px.byte 를 4 byte 만큼 전송 
    Serial.write(py.bytes, sizeof(px.bytes));
    Serial.write(ptheta.bytes, sizeof(px.bytes));
    Serial.write(tx_byte);

    unsigned long endTime = micros();   // 2. 끝 시간 기록 -> Task Frequency 설정 위함 
    
    Serial.write((uint8_t*)&endTime, sizeof(endTime));

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

