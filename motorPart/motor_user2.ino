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

int success_move = false;

MPU6050 mpu(Wire);

typedef struct _FloatPacket {
  float value;
  uint8_t bytes[4];
} FloatPacket_t;


//--- 2. Queue 핸들 정의 ---
QueueHandle_t xMotorQueue;

TaskHandle_t Motor_Control;
TaskHandle_t Motor_ESTOP;

//--- 3. Queue로 보낼 데이터 타입 정의 ---
typedef enum {
  CMD_STOP, CMD_UP, CMD_DOWN, CMD_LEFT, CMD_RIGHT, CMD_MAPPING, CMD_MAPPED, CMD_INVALID
} MotorCommand_t; // MotorCommand_t 라는 새로운 타입을 만듦

volatile MotorCommand_t g_currentMotorState = CMD_STOP;
volatile bool g_isObstacleDetected = false; // E-STOP 상태 플래그
volatile uint8_t g_robot_status;
volatile bool g_rx_error = false;

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
    case CMD_MAPPING:return "MAPPING";
    case CMD_MAPPED: return "MAPPED";
    default:         return "INVALID";
  }
}

void ISR_Encoder_A(){
  g_ENCODER_LEFT_COUNT++;  
}

void ISR_Encoder_B(){
  g_ENCODER_RIGHT_COUNT++;
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
      
      //★ 여기가 핵심 ★
      if (cmd_to_send != CMD_INVALID) {
         g_rx_error = false; 
         xQueueOverwrite(xMotorQueue, &cmd_to_send);
      } else {
         g_rx_error = true;
      }
    }
    // 이 태스크를 잠시(100ms) 재워서 다른 태스크(MotorTask)가 실행될 시간을 줌
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

float g_target_heading_deg;
float TARGET_CM = 20;

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
          g_target_heading_deg = 0.0f;
          rotate_sequence();
          move_sequence(TARGET_CM);
          break;
        case CMD_DOWN:
          g_target_heading_deg = 180.0f;
          rotate_sequence();
          move_sequence(TARGET_CM);
          break;
        case CMD_LEFT:
          g_target_heading_deg = -90.0f;
          rotate_sequence();
          move_sequence(TARGET_CM);
          break;
        case CMD_RIGHT:
          g_target_heading_deg = 90.0f;
          rotate_sequence();
          move_sequence(TARGET_CM);
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
  uint8_t tx_byte = 0x00; 

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

    ptheta.value = g_robot_theta;
    

    if (isEmergency) tx_byte = 0x02;
    else if (g_rx_error) tx_byte = 0x04;
    else if (success_move) tx_byte = 0x01; 

    if(tx_byte != 0x00) {
      Serial.write(0xAA); 
      Serial.write(0xBB);
      Serial.write(tx_byte);
      isEmergency = false;
      g_rx_error = false;
      success_move = false;
      tx_byte = 0x00;
    }

    unsigned long endTime = micros();   // 2. 끝 시간 기록 -> Task Frequency 설정 위함 
    
    // Serial.write((uint8_t*)&endTime, sizeof(endTime));

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// 전역 변수 (PID 제어 변수 선언)
double Input, Output, Setpoint;
// Kp, Ki, Kd 값은 실험을 통해 튜닝해야 함
double Kp = 1.5, Ki = 0.01, Kd = 0.5; 
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

void rotate_sequence() {
    Setpoint = g_target_heading_deg;
    myPID.SetMode(AUTOMATIC);
    myPID.SetOutputLimits(-255, 255); 
    
    // 타임아웃 추가 (예: 3초 지나면 강제 종료)
    unsigned long start_time = millis();

    while (fabs(g_target_heading_deg - mpu.getAngleZ()) > 5.0) {
        
        // // 3초 타임아웃 (무한루프 방지) 이러면 돌다가 정확하지 못한 곳에서 멈춰서 이상한곳으로 가는거 아님 ? 
        // if (millis() - start_time > 3000) break;

        mpu.update();

        Input = mpu.getAngleZ();
        myPID.Compute(); 
        
        // 1. 절대값 변환
        int abs_output = abs(Output);
        int pwm_val = 0;

        // 2. 데드존 매핑 (공식 적용)
        // PID가 조금이라도 출력(1 이상)을 내면, 모터는 즉시 100부터 시작해서 최대 200까지 비례해서 증가
        if (abs_output > 0) {
            pwm_val = map(abs_output, 0, 255, 100, 200); 
        }

        // 3. 방향 제어
        if (Output > 0) {
            Motor_RIGHT(pwm_val);
        } else if (Output < 0) {
            Motor_LEFT(pwm_val);
        } else {
            Motor_STOP();
        }

        vTaskDelay(20 / portTICK_PERIOD_MS); 
    }
    Motor_STOP();
}

//이동
void move_sequence(float target_cm) {
  
  // 2. 목표 펄스 계산 ( 10cm / 펄스당거리 )
  long target_pulses = (long)(target_cm / DIST_PER_PULSE);
  taskENTER_CRITICAL();
  long start_L = g_ENCODER_LEFT_COUNT; 
  long start_R = g_ENCODER_RIGHT_COUNT;
  taskEXIT_CRITICAL();
  long start_count = (start_L + start_R) / 2 ;
  unsigned long start_time = millis();

  Motor_UP();

  while (true) {

      if (g_isObstacleDetected) {
        Motor_STOP();
        return;
      }


      if (millis() - start_time > 5000) {
        break; 
      }
      // 3. 현재 이동 거리 계산 (현재 평균 - 시작 평균)
      long curr_L, curr_R;
      taskENTER_CRITICAL();
      curr_L = g_ENCODER_LEFT_COUNT;
      curr_R = g_ENCODER_RIGHT_COUNT;
      taskEXIT_CRITICAL();
      long curr_avg = (curr_L + curr_R) / 2;
      // 목표 도달 체크
      if (abs(curr_avg - start_count) >= target_pulses) {
          break;
      }

      vTaskDelay(10 / portTICK_PERIOD_MS); 
  }
  Motor_STOP();
  success_move = true;
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
  motor_speed(180);
}

void Motor_STOP() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, LOW);

  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, LOW);
  motor_speed(150);
}

void Motor_LEFT(int spd) {
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);

  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, HIGH);
  motor_speed(spd);
}

void Motor_RIGHT(int spd) {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);

  digitalWrite(MOTOR_B_IN1, HIGH);
  digitalWrite(MOTOR_B_IN2, LOW);
  motor_speed(spd);
}

void Motor_DOWN() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);

  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, HIGH);
  motor_speed(150);
}