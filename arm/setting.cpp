#include <wiringPi.h>
#include <softPwm.h>
#include <unistd.h> // for sleep
#include <math.h>
// 서보 핀 번호 (wiringPi 기준)
const int SERVO1 = 9;  // GPIO 22
const int SERVO2 = 21;  // GPIO 24
const int SERVO3 =22;  // GPIO 25
const int SERVO5 = 10; // GPIO 8

const int NATURAL_ANGLE = 90;

// 하단: 9번
// 중간 21
//상단 22
//엔드 10

// map 함수 먼저 정의
int map(int x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
// 아두이노의 write(각도)를 PWM 값으로 변환
int angleToPWM(int angle) {
    return map(angle, 0, 180, 5, 25);  // softPwm은 0~100의 범위를 가짐. 1ms~2ms = 5~25
}


int main() {
    wiringPiSetup();

    // 소프트 PWM 초기화
    softPwmCreate(SERVO1, 0, 100);
    softPwmCreate(SERVO2, 0, 100);
    softPwmCreate(SERVO3, 0, 100);
    softPwmCreate(SERVO5, 0, 100);

    int pwmValue = angleToPWM(NATURAL_ANGLE);

    softPwmWrite(SERVO1, pwmValue);
    softPwmWrite(SERVO2, pwmValue);
    softPwmWrite(SERVO3, pwmValue);
    softPwmWrite(SERVO5, pwmValue);

    // 서보가 움직일 시간을 주기 위해 sleep
    delay(500); // 500ms

    // loop 없음 (아두이노의 loop 함수 없음)
    return 0;
}
