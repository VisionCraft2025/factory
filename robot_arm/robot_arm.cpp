#include <wiringPi.h>
#include <softPwm.h>
#include <unistd.h>
#include <iostream>
#include <math.h>

using namespace std;

// 서보 핀 번호 (wiringPi 기준)
const int SERVO1 = 3;  // GPIO 22, 디지털핀 3 - Base
const int SERVO2 = 5;  // GPIO 24, 디지털핀 5 - Shoulder  
const int SERVO3 = 6;  // GPIO 25, 디지털핀 6 - Elbow
const int SERVO5 = 10; // GPIO 8, 디지털핀 10 - Gripper

const int NATURAL_ANGLE = 90;
int tt = 8; // 서보 이동 딜레이
int x = 0, y = 0, z = 0; // 서보 각도 변수

int map(int x, int in_min, int in_max, int out_min, int out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// 각도를 PWM 값으로 변환
int angleToPWM(int angle)
{
    return map(angle, 0, 180, 5, 25);  // 1ms~2ms = 5~25
}

// 서보 모터 이동
void moveServo(int pin, int angle)
{
    softPwmWrite(pin, angleToPWM(angle));
    delay(tt);
}

// 초기 위치에서 A 설비 위치로 이동
void natural_A()
{
    for (y = NATURAL_ANGLE; y >= 75; y--)
        moveServo(SERVO2, y);

    for (z = NATURAL_ANGLE; z <= 100; z++)
        moveServo(SERVO3, z);

    for (x = NATURAL_ANGLE; x >= 30; x--)
        moveServo(SERVO1, x);
}

// A 설비에서 초기 위치로 복귀
void A_natural()
{
    for (x = 30; x <= NATURAL_ANGLE; x++)
        moveServo(SERVO1, x);

    for (z = 100; z >= NATURAL_ANGLE; z--)
        moveServo(SERVO3, z);

    for (y = 75; y <= NATURAL_ANGLE; y++)
        moveServo(SERVO2, y);
}

// 초기 위치에서 B설비로 이동 -> 이때 그리퍼 동작함
void natural_B()
{
    for (y = NATURAL_ANGLE; y >= 50; y--)
        moveServo(SERVO2, y);

    for (z = NATURAL_ANGLE; z <= 120; z++)
        moveServo(SERVO3, z);

    for (x = NATURAL_ANGLE; x <= 145; x++)
        moveServo(SERVO1, x);

    delay(200);
    moveServo(SERVO5, 180); // 그리퍼 열기
    delay(500);
    moveServo(SERVO5, 90);  // 그리퍼 닫기
}

// B설비에서 초기 위치로 복귀
void B_natural()
{
    for (x = 145; x >= NATURAL_ANGLE; x--)
        moveServo(SERVO1, x);

    for (y = 50; y <= NATURAL_ANGLE; y++)
        moveServo(SERVO2, y);

    for (z = 120; z >= NATURAL_ANGLE; z--)
        moveServo(SERVO3, z);
}

// 초기 세팅
void initializeServos()
{
    cout << "서보 모터 초기화 중..." << endl;
    
    softPwmCreate(SERVO1, 0, 100);
    softPwmCreate(SERVO2, 0, 100);
    softPwmCreate(SERVO3, 0, 100);
    softPwmCreate(SERVO5, 0, 100);
    
    // 모든 서보를 초기 위치로 세팅하고
    int pwmValue = angleToPWM(NATURAL_ANGLE);
    softPwmWrite(SERVO1, pwmValue);
    softPwmWrite(SERVO2, pwmValue);
    softPwmWrite(SERVO3, pwmValue);
    softPwmWrite(SERVO5, pwmValue);
    
    // 이 시간동안 서보가 움직일 수 있도록 딜레이 시간 주기
    delay(2000);
    cout << "초기화 완료" << endl;
}

int main()
{
    if (wiringPiSetup() == -1)
    {
        cout << "wiringPi 초기화 실패" << endl;
        return 1;
    }
    
    // 서보 모터 초기화
    initializeServos();
    
    cout << "동작 시작..." << endl;
    
    // 메인 동작 루프
    while (true)
    {
        natural_A();
        delay(4000);

        A_natural();
        natural_B();
        B_natural();

        delay(1000);
    }
    
    return 0;
}
