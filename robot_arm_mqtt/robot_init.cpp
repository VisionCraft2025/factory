#include <wiringPi.h>
#include <unistd.h>
#include <iostream>

#define SERVO_PIN 10  // wiringPi 핀 10 = GPIO 8번 (엔드모터)

void set_servo_angle(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 220) angle = 220;

    // 안정된 펄스 범위로 제한
    int pulse_width = 1000 + (angle * 1000) / 180;
    if (pulse_width > 2200) pulse_width = 2200;

    std::cout << "Moving servo to " << angle << " degrees" << std::endl;

    // 안정된 PWM 신호 전송
    for (int i = 0; i < 100; ++i) {
        digitalWrite(SERVO_PIN, HIGH);
        delayMicroseconds(pulse_width);
        digitalWrite(SERVO_PIN, LOW);
        delay(20);  // 20ms 고정 주기
    }
}

int main() {
    if (wiringPiSetup() == -1) {
        std::cerr << "wiringPi init failed!" << std::endl;
        return 1;
    }

    pinMode(SERVO_PIN, OUTPUT);

    int angles[] = {0, 90, 180, 220};
    
    while (true) {
        for (int i = 0; i < 4; i++) {
            set_servo_angle(angles[i]);
            delay(3000);  // 3초 대기
        }
    }
    return 0;
}
