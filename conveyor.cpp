#include <iostream>
#include <wiringPi.h>
#include <softPwm.h>
#include <unistd.h> // usleep

// wiringPi 기준 핀 번호 사용
const int STEP_PIN = 0;     // BCM 17
const int DIR_PIN = 2;      // BCM 27
const int ENABLE_PIN = 3;   // BCM 22
const int SERVO_PIN = 21;   // BCM 5

// map 함수 정의
long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void moveServo(int angle) {
    int pwm = map(angle, 0, 180, 5, 25);
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
    // wiringPi 초기화
    wiringPiSetup();
    softPwmCreate(SERVO_PIN, 0, 200); // 0~200 범위로 PWM 생성

    // 스텝모터 핀 설정
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    pinMode(ENABLE_PIN, OUTPUT);

    digitalWrite(ENABLE_PIN, LOW); // 모터 활성화
    digitalWrite(DIR_PIN, HIGH);   // 정방향 설정

    std::cout << "스텝모터 정방향 2000스텝" << std::endl;
    for (int i = 0; i < 20000; ++i) {
        digitalWrite(STEP_PIN, HIGH);
        usleep(1000); // 1ms
        digitalWrite(STEP_PIN, LOW);
        usleep(1000);
    }

    digitalWrite(ENABLE_PIN, HIGH); // 모터 비활성화

    std::cout << "서보모터 0~180도 왕복" << std::endl;
    sweepServo();

    std::cout << "모든 동작 완료!" << std::endl;
    return 0;
}
