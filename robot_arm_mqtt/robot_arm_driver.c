#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("VIsionCraft_jang");
MODULE_DESCRIPTION("Robot Arm Driver with Servo Control");

#define DEVICE_NAME "robot_arm"
#define BUF_LEN 16
#define NUM_SERVOS 4
#define PWM_PERIOD_US 20000
#define PWM_MIN_US 1000
#define PWM_MAX_US 2000
#define DEADBAND_DEGREES 2

#define SEQUENCE_DELAY 2000

#define GPIO_BASE 512

// 모터별 pwm 값
#define PWM_MIN_MG996R 500
#define PWM_MAX_MG996R 2500
#define PWM_MIN_MG90S 1000
#define PWM_MAX_MG90S 2000

// 서보모터 4개 -> 하단, 중단, 상단, 엔드 모터
static int servo_pins[NUM_SERVOS] =
    {
        GPIO_BASE + 22,
        GPIO_BASE + 24,
        GPIO_BASE + 25,
        GPIO_BASE + 8};

/*
만약에 로봇 팔의 각 관절을 사용자가 디테일하게 수동으로 설정하고 싶을 때 사용하기 위해
서보모터의 상태를 알려주는 구조체 만듦 -> 이건 일단 옵션!
*/
struct servo_state
{
    int current_angle; // 현재 각도
    int target_angle;  // 가고 싶은 각도
    bool moving;       // 움직이고 있는지 여부
};

static struct servo_state servos[NUM_SERVOS];
static struct task_struct *servo_thread;  // 서보모터를 움직이게 하는 스레드가 누구인지 기억하기 위해 메모
static struct task_struct *motion_thread; // 자동으로 동작하는 시퀀스 스레드
static int major_num;                     // 디바이스 파일의 메이저 번호
static char command_buffer[BUF_LEN];

static bool auto_mode = false; // 자동 모드인지 -> 자동모드는 한번 실행하면 유튜브에서 봤던 것처럼 자동으로 연속적으로 동작
static bool waiting_for_go = false; // come 동작 완료 후 go 명령을 기다리는 상태
static bool come_mode = false; // come 명령을 실행하는 모드
static bool go_mode = false; // go 명령을 실행하는 모드

static int current_sequence = 0; // 자동 모드 시 현재 수행단계(총 4단계: 0 ~ 4)
                                 // -> ex) 0단계: 피더쪽으로 돌려서 물건 받기, 1단계: 컨베이어 벨트로 돌려서 물건 놓기, ...

// 각도를 입력받으면, 그걸 펄스폭(시간)으로 변환하는 함수
static int angle_to_pwm_us(int angle, int servo_id)
{
    if (angle < 0)
        angle = 0;
    if (angle > 250)
        angle = 250;

    int min_us, max_us;

    if (servo_id == 0 || servo_id == 1) {
        // MG996R
        min_us = PWM_MIN_MG996R;
        max_us = PWM_MAX_MG996R;
    } else {
        // MG90S
        min_us = PWM_MIN_MG90S;
        max_us = PWM_MAX_MG90S;
    }

    return min_us + (angle * (max_us - min_us)) / 180;
}



// 특정 서보모터에 PWM 신호를 주는 함수
static void set_servo_pwm(int servo_id, int angle)
{
    static int last_angles[NUM_SERVOS] = { 90, 90, 90, 90 };  // 초기값을 명시적으로 90
    
    if (abs(last_angles[servo_id] - angle) >= DEADBAND_DEGREES)
    {
        last_angles[servo_id] = angle;

        // 각도를 펄스폭(시간)으로 변환 -> 시간이 길면 길수록 모터가 돌아가는 시간 길어짐 -> 더 많은 각도로 돌아감
        int pulse_width_us = angle_to_pwm_us(angle, servo_id);

        gpio_set_value(servo_pins[servo_id], 1);           // GPIO HIGH -> 모터 동작 시작
        usleep_range(pulse_width_us, pulse_width_us + 10); // 펄스폭만큼 대기 -> 이때 모터가 돌아감
        gpio_set_value(servo_pins[servo_id], 0);           // GPIO LOW -> 모터 동작 중지
    }  
}


// PWM 신호를 연속적으로 생성하는 스레드
static int servo_pwm_thread(void *data)
{
    // 스레드 종료 신호가 올 때까지 계속 반복
    while (!kthread_should_stop())
    {
        // 모든 서보모터에 PWM 신호 전송하기
        for (int i = 0; i < NUM_SERVOS; i++)
            set_servo_pwm(i, servos[i].current_angle);

        // 서보모터는 20ms 주기로 같은 신호를 계속 줘야 함 -> 안 그러면 멈추거나 헛돈다..
        // 각 서보모터에 PWM 신호를 주고나면, 다음 20ms 주기를 맞추기 위해 잠시 기다려야 함
        usleep_range(PWM_PERIOD_US - 8000, PWM_PERIOD_US - 7000);
    }
    return 0;
}

// 모터가 너무 급발진해서 좀 부드럽게 움직이게 하는 함수
static void move_servo_smooth(int servo_id, int target_angle, int delay_ms)
{
    int current_pos = servos[servo_id].current_angle;
    int step = (target_angle > current_pos) ? 1 : -1;

    while (current_pos != target_angle)
    {
        current_pos += step; // 1도 씩 이동
        servos[servo_id].current_angle = current_pos;
        msleep(delay_ms); // 부드러운 동작을 위해 딜레이
    }


    if (servos[servo_id].current_angle == target_angle)
    set_servo_pwm(servo_id, target_angle);
}

// 자동으로 움직이는 시퀀스를 실행하는 커널 스레드
static int robot_motion_thread(void *data)
{
    // 스레드 종료 신호 올 때까지 계속 반복하기
    while (!kthread_should_stop())
    {
        if (auto_mode)
        {
            // 기존 자동 모드 동작 (전체 시퀀스)
            // 0. servo3
            move_servo_smooth(3, 0, 1);

            // 1. servo0
            move_servo_smooth(0, 260, 1);

            // 2. servo2
            move_servo_smooth(2, 120, 1);
            move_servo_smooth(2, 30, 1);

            // 3. servo1 
            move_servo_smooth(1, 60, 1); //60도 ㄱㅊ

            // 4. 5초 대기
            msleep(5000);

            // 5. servo1 
            move_servo_smooth(1, 120, 1); 

            // 6. servo0
            move_servo_smooth(0, 20, 1); //되돌아가기

            // 7. servo1 
            move_servo_smooth(1, 90, 1); //60도 ㄱㅊ

            // 8. servo2
            move_servo_smooth(2, 110, 1);

            // 9. servo3 상자 버리기
            move_servo_smooth(3, 220, 1);

            // 10. 5초 대기
            msleep(5000);   

            // 11. servo1 
            move_servo_smooth(1, 120, 1); 

            // 12. servo2
            move_servo_smooth(2, 160, 1);

            // 각도 유지 강제 PWM 재전송 (헛돎 방지)
            for (int i = 0; i < NUM_SERVOS; i++) {
                set_servo_pwm(i, servos[i].current_angle);
            }

            msleep(500);
        }
        else if (come_mode)
        {
            // 첫 번째 부분: come 명령 실행 (물건 집기)
            // 0. servo3
            move_servo_smooth(3, 0, 1);

            // 1. servo0
            move_servo_smooth(0, 260, 1);

            // 2. servo2
            move_servo_smooth(2, 120, 1);
            move_servo_smooth(2, 30, 1);

            // 3. servo1 
            move_servo_smooth(1, 60, 1); //60도 ㄱㅊ
           
            // 명령 실행 후 go 명령 기다리는 모드로 전환
            come_mode = false;
            waiting_for_go = true; // go 명령을 기다림
            printk(KERN_INFO "Robot Arm: COME action completed, waiting for GO command\n");
            
            // 각도 유지 강제 PWM 재전송 (헛돎 방지)
            for (int i = 0; i < NUM_SERVOS; i++) {
                set_servo_pwm(i, servos[i].current_angle);
            }
        }
        else if (go_mode)
        {
             // 5. servo1 
            move_servo_smooth(1, 120, 1);
            

            // 두 번째 부분: go 명령 실행 (물건 놓기)
            // 6. servo0
            move_servo_smooth(0, 20, 1); //되돌아가기

            // 7. servo1 
            move_servo_smooth(1, 90, 1); //60도 ㄱㅊ

            // 8. servo2
            move_servo_smooth(2, 110, 1);

            // 9. servo3 상자 버리기
            move_servo_smooth(3, 220, 1);

            // 10. 5초 대기
            msleep(5000);   

            // 11. servo1 
            move_servo_smooth(1, 120, 1); 

            // 12. servo2
            move_servo_smooth(2, 160, 1);

            move_servo_smooth(3, 0, 1);

            // 1. servo0
            move_servo_smooth(0, 260, 1);

            // 2. servo2
            move_servo_smooth(2, 120, 1);
            move_servo_smooth(2, 30, 1);

            // 3. servo1 
            move_servo_smooth(1, 60, 1); //60도 ㄱㅊ

            
            // 명령 실행 후 다시 on 명령 실행 (자동으로 come 동작 시작)
            go_mode = false;
            waiting_for_go = false;
            come_mode = true; // 다시 come 동작 시작
            printk(KERN_INFO "Robot Arm: GO action completed, automatically starting COME action again\n");
            
            // 각도 유지 강제 PWM 재전송 (헛돎 방지)
            for (int i = 0; i < NUM_SERVOS; i++) {
                set_servo_pwm(i, servos[i].current_angle);
            }
        }
        else
            msleep(100); // 모든 모드가 꺼져있으면 100ms마다 확인
    }
    return 0;
}

// 사용자가 디바이스 파일에 명령어를 입력할 때 호출되는 함수
static ssize_t robot_arm_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    int servo_id, angle;

    // 명령어가 주어진 버퍼보다 길면 에러처리
    if (len > BUF_LEN - 1)
        return -1;

    // 사용자 공간에서 사용자가 입력한 명령어를 커널 공간으로 복사하기
    if (copy_from_user(command_buffer, buf, len))
        return -1;

    command_buffer[len] = '\0';

    // 명령어 처리
    if (strncmp(command_buffer, "on", 2) == 0)
    {
        // on 명령은 바로 come 동작 실행
        auto_mode = false;
        waiting_for_go = false;
        come_mode = true;
        go_mode = false;
        current_sequence = 0;
        printk(KERN_INFO "Robot Arm: ON command - executing COME action\n");
    }
    else if (strncmp(command_buffer, "off", 3) == 0)
    {
        // 모든 모드 비활성화
        auto_mode = false;
        waiting_for_go = false;
        come_mode = false;
        go_mode = false;
        // 모든 서보모터를 중앙으로 돌려서 초기화시키기 -> 지금은 중앙을 90도로 설정함
        move_servo_smooth(3, 90, 1);
        move_servo_smooth(2, 100, 1);
        move_servo_smooth(1, 90, 1);
        move_servo_smooth(0, 0, 1);
        printk(KERN_INFO "Robot Arm: All modes deactivated\n");
    }
    else if (strncmp(command_buffer, "init", 4) == 0)
    {
        // 모든 서보모터를 중앙으로 돌려서 초기화시키기
        move_servo_smooth(3, 220, 1); //엔드
        move_servo_smooth(2, 100, 1);
        move_servo_smooth(1, 90, 1);
        move_servo_smooth(0, 90, 1); // 1
        printk(KERN_INFO "Robot Arm: Initialized to default position\n");
    }
    else if (strncmp(command_buffer, "come", 4) == 0)
    {
        // come 명령 - 어떤 모드에서도 실행 가능
        waiting_for_go = false;
        come_mode = true;
        go_mode = false;
        printk(KERN_INFO "Robot Arm: Executing COME command\n");
    }
    else if (strncmp(command_buffer, "go", 2) == 0)
    {
        // go 명령 - waiting_for_go 모드에서만 동작
        if (waiting_for_go) {
            waiting_for_go = false;
            come_mode = false;
            go_mode = true;
            printk(KERN_INFO "Robot Arm: Executing GO command\n");
        } else {
            printk(KERN_INFO "Robot Arm: GO command ignored - not waiting for GO\n");
        }
    }
    else if (sscanf(command_buffer, "servo%d %d", &servo_id, &angle) == 2)
    {
        // 사용자가 수동으로 조정하고 싶을 때를 위해 일단 구현함
        // "servo0 90" 이런식인데, 이렇게 하면 하단모터가 90도 돌아감

        if (servo_id >= 0 && servo_id < NUM_SERVOS && angle >= 0 && angle <= 250) {
        servos[servo_id].current_angle = angle;
        set_servo_pwm(servo_id, angle);  // 직접 PWM 신호 보내기
    }
    }
    else
    {
        // 명령어 잘못 입력한 경우 -> 강사님이 printk는 리소스 많이 잡아먹는다고 하셔서 따로 안 적음...
    }

    return len;
}

// 사용자가 디바이스 파일에서 상태를 읽을 때 호출되는 함수
static ssize_t robot_arm_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    char status[256];
    int status_len;

    // 이미 읽었으면 EOF 반환 -> 중복으로 읽는거 방지
    if (*offset > 0)
        return 0;

    status_len = snprintf(status, sizeof(status),
                          "자동 모드: %s\nGo 대기 모드: %s\nCome 모드: %s\nGo 모드: %s\n현재 동작단계: %d\n서보모터 각도(하단/중단/상단/그리퍼): %d %d %d %d\n",
                          auto_mode ? "ON" : "OFF", 
                          waiting_for_go ? "ON" : "OFF",
                          come_mode ? "ON" : "OFF",
                          go_mode ? "ON" : "OFF",
                          current_sequence,
                          servos[0].current_angle, servos[1].current_angle, servos[2].current_angle, servos[3].current_angle);

    if (len < status_len)
        return -1;

    if (copy_to_user(buf, status, status_len))
        return -1;

    *offset = status_len;
    return status_len;
}

// 캐릭터 디바이스 파일의 명령어 -> 사용자 프로그램이 /dev/robot_arm에 읽거나 쓸 때 호출될 함수들
static struct file_operations fops =
    {
        .owner = THIS_MODULE,
        .write = robot_arm_write, // write할 때
        .read = robot_arm_read,   // read할 때
};

// insmod할 때 호출되는 초기화 함수
static int __init robot_arm_init(void)
{
    int ret;

    // 캐릭터 디바이스 드라이버 등록하기
    major_num = register_chrdev(0, DEVICE_NAME, &fops);

    // 메이저 번호가 이상하면 오류리턴
    if (major_num < 0)
        return major_num;

    for (int i = 0; i < NUM_SERVOS; i++)
    {
        ret = gpio_request(servo_pins[i], "robot_arm_servo");

        if (ret)
        {
            for (int j = 0; j < i; j++)
                gpio_free(servo_pins[j]);

            unregister_chrdev(major_num, DEVICE_NAME);

            return -1;
        }

        // GPIO를 출력 모드로 설정 -> 초기값은 LOW로 설정 -> 그래서 0으로 설정
        gpio_direction_output(servo_pins[i], 0);

        // 서보모터들 상태 초기화 -> 모두 90도 중앙 위치로 초기화
        move_servo_smooth(3, 220, 1); // 엔드 모터
        move_servo_smooth(2, 100, 1);
        move_servo_smooth(1, 90, 1);
        move_servo_smooth(0, 90, 1);
        servos[i].moving = false;
    }

    // PWM을 생성하는 스레드 생성하고 바로 시작
    servo_thread = kthread_run(servo_pwm_thread, NULL, "robot_arm_pwm");
    if (IS_ERR(servo_thread))
        return PTR_ERR(servo_thread);

    // 자동 동작 시퀀스 스레드 시작
    motion_thread = kthread_run(robot_motion_thread, NULL, "robot_arm_motion");
    if (IS_ERR(motion_thread))
    {
        kthread_stop(servo_thread);
        return PTR_ERR(motion_thread);
    }

    return 0;
}

// rmmod할 때 호출되는 함수
static void __exit robot_arm_exit(void)
{
    // rmmod 하면 그동안 했던 것들 싹다 종료하고 정리하는게 나음 -> 자동모드 끄자
    auto_mode = false;

    // 커널 스레드 종료
    if (motion_thread)
        kthread_stop(motion_thread);
    if (servo_thread)
        kthread_stop(servo_thread);

    // GPIO 정리
    for (int i = 0; i < NUM_SERVOS; i++)
    {
        gpio_set_value(servo_pins[i], 0); // GPIO 핀들 다 LOW로 세팅
        gpio_free(servo_pins[i]);         // GPIO 핀 해제
    }

    // 디바이스 등록 해제
    unregister_chrdev(major_num, DEVICE_NAME);
}

// 모듈 등록
module_init(robot_arm_init); // insmod할 때 호추ㄹ
module_exit(robot_arm_exit); // rmmod할 떄 호출