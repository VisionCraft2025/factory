#include <wiringPi.h>
#include <unistd.h>

// 핀 번호 (WiringPi 번호)
#define IN1 14
#define IN2 15
#define IN3 16
#define IN4 17

#define STEPS 2048
#define DELAY_MS 5 // 속도 조절

int sequence[8][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

void stepMotor(int step) {
    digitalWrite(IN1, sequence[step][0]);
    digitalWrite(IN2, sequence[step][1]);
    digitalWrite(IN3, sequence[step][2]);
    digitalWrite(IN4, sequence[step][3]);
}

int main() {
    wiringPiSetupGpio(); // BCM 핀 번호 사용

    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    while (true) {
        for (int i = 0; i < STEPS; i++) {
            stepMotor(i % 8);
            delay(DELAY_MS);
        }
    }

    return 0;
}
