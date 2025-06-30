#include <wiringPi.h>
#include <softPwm.h>

const int SERVO_PIN = 21; // wiringPi 번호 21 = GPIO 5 = Physical 29번

int map(int x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int angleToPWM(int angle) {
    return map(angle, 0, 180, 5, 25); // 5~25% → 1ms~2ms pulse
}

int main() {
    wiringPiSetup();
    softPwmCreate(SERVO_PIN, 0, 100); // 초기값 0, 최대값 100

    while (1) {
        softPwmWrite(SERVO_PIN, angleToPWM(0));
        delay(1000);
        softPwmWrite(SERVO_PIN, angleToPWM(90));
        delay(1000);
        softPwmWrite(SERVO_PIN, angleToPWM(180));
        delay(1000);
    }

    return 0;
}
