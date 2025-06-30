#include <wiringPi.h>
#include <softPwm.h>
#include <unistd.h>
#include <iostream>

using namespace std;

//const int SERVO1 = 3;   // Base
//const int SERVO2 = 5;   // Shoulder
//const int SERVO3 = 6;   // Elbow
//const int SERVO5 = 10;  // Gripper



const int SERVO1 = 9;  // GPIO 22, 디지털핀 3
const int SERVO2 = 21;  // GPIO 24 디지털핀 5
const int SERVO3 =22;  // GPIO 25  디지털핀6
const int SERVO5 = 10; // GPIO 8 디지털핀 10



int tt = 8;
int x = 0, y = 0, z = 0;
const int natural = 90;

int mapAngleToPWM(int angle) {
    return (angle - 0) * (25 - 5) / (180 - 0) + 5;
}

void moveServo(int pin, int angle) {
    softPwmWrite(pin, mapAngleToPWM(angle));
    delay(tt);
}

void natural_conv1() {
    for (y = natural; y >= 75; y--) moveServo(SERVO2, y);
    for (z = natural; z <= 100; z++) moveServo(SERVO3, z);
    for (x = natural; x >= 30; x--) moveServo(SERVO1, x);
}

void conv1_natural() {
    for (x = 30; x <= natural; x++) moveServo(SERVO1, x);
    for (z = 100; z >= natural; z--) moveServo(SERVO3, z);
    for (y = 75; y <= natural; y++) moveServo(SERVO2, y);
}

void natural_conv2() {
    for (y = natural; y >= 50; y--) moveServo(SERVO2, y);
    for (z = natural; z <= 120; z++) moveServo(SERVO3, z);
    for (x = natural; x <= 145; x++) moveServo(SERVO1, x);

    delay(200);
    moveServo(SERVO5, 180);
    delay(500);
    moveServo(SERVO5, 90);
}

void conv2_natural() {
    for (x = 145; x >= natural; x--) moveServo(SERVO1, x);
    for (y = 50; y <= natural; y++) moveServo(SERVO2, y);
    for (z = 120; z >= natural; z--) moveServo(SERVO3, z);
}

int main() {
    wiringPiSetup();

    softPwmCreate(SERVO1, 0, 100);
    softPwmCreate(SERVO2, 0, 100);
    softPwmCreate(SERVO3, 0, 100);
    softPwmCreate(SERVO5, 0, 100);

    cout << "start" << endl;

    moveServo(SERVO1, natural);
    moveServo(SERVO2, natural);
    moveServo(SERVO3, natural);
    moveServo(SERVO5, natural);

    delay(2000);

    while (true) {
        natural_conv1();
        delay(4000);

        conv1_natural();
        natural_conv2();
        conv2_natural();
    }

    return 0;
}
