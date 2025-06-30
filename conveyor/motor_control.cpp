#include <wiringPi.h>
#include <softPwm.h>
#include <unistd.h>
#include <iostream>

// map 함수 직접 구현
long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// BCM 핀 번호 사용
#define STEP_PIN 12  // BCM GPIO 10 -> wiringPi 12
#define DIR_PIN 13   // BCM GPIO 9  -> wiringPi 13
#define EN_PIN 10    // BCM GPIO 8  -> wiringPi 10

#define SERVO_PIN 21 // BCM GPIO 5  -> wiringPi 21

void setup() {
    wiringPiSetup(); // wiringPi 초기화

    // 스텝모터 핀 설정
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    pinMode(EN_PIN, OUTPUT);

    digitalWrite(EN_PIN, LOW); // 스텝모터 활성화

    // 서보모터 핀 설정 (소프트 PWM)
    softPwmCreate(SERVO_PIN, 0, 200); // 0~200 (20ms 주기)
}

void stepMotor(int steps, int delayMicros, bool clockwise = true) {
    digitalWrite(DIR_PIN, clockwise ? HIGH : LOW);
    for (int i = 0; i < steps; i++) {
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(delayMicros);
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(delayMicros);
    }
}

void moveServo(int angle) {
    int pwm = map(angle, 0, 180, 5, 25); // 0~180도를 PWM 범위로 변환
    softPwmWrite(SERVO_PIN, pwm);
}

void sweepServo() {
    for (int angle = 0; angle <= 180; angle++) {
        moveServo(angle);
        delay(10);
    }
    delay(500);
    for (int angle = 180; angle >= 0; angle--) {
        moveServo(angle);
        delay(10);
    }
    delay(500);
}

int main() {
    setup();

    std::cout << "스텝모터 정방향 200스텝" << std::endl;
    stepMotor(200, 1000); // 200스텝, 1000us 속도

    delay(1000);

    std::cout << "서보모터 0->180도 왕복" << std::endl;
    sweepServo();

    std::cout << "종료됨" << std::endl;

    return 0;
}
