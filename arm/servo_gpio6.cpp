#include <wiringPi.h>
#include <softPwm.h>

const int SERVO_PIN = 22; // wiringPi 번호 = GPIO 6번 = Physical 31번

// 아두이노 스타일 map 함수
int map(int x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// 각도를 softPwm duty로 변환
int angleToPWM(int angle) {
    return map(angle, 0, 180, 5, 25); // 5~25% duty cycle → 약 1~2ms 펄스
}

int main() {
    wiringPiSetup(); // wiringPi 핀 번호 사용
    softPwmCreate(SERVO_PIN, 0, 100); // 핀, 초기값, 최대값(100)

    while (true) {
        softPwmWrite(SERVO_PIN, angleToPWM(0));
        delay(1000);
        softPwmWrite(SERVO_PIN, angleToPWM(90));
        delay(1000);
        softPwmWrite(SERVO_PIN, angleToPWM(180));
        delay(1000);
    }

    return 0;
}
