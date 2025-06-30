#include <wiringPi.h>
#include <softPwm.h>
#include <iostream>

using namespace std;

// 서보모터가 연결된 핀 (wiringPi 기준 번호)
const int SERVO_PIN = 10;

// 아두이노 map() 함수와 동일
int map(int x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// 각도를 PWM값으로 변환해서 서보에 전달
void moveServo(int angle) {
    int pwm = map(angle, 0, 180, 5, 25); // SG90: 0도=1ms(5), 180도=2ms(25)
    softPwmWrite(SERVO_PIN, pwm);
    cout << "Moved to angle: " << angle << " (" << pwm << "% duty)" << endl;
    delay(1000);
}

int main() {
    wiringPiSetup();
    softPwmCreate(SERVO_PIN, 0, 100); // 초기값 0, 범위 100

    cout << "Starting servo test on pin " << SERVO_PIN << endl;

    moveServo(0);
    moveServo(90);
    moveServo(180);

    cout << "Test done." << endl;
    return 0;
}
