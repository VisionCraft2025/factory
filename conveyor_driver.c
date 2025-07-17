#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/delay.h> 
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/sched.h> 

MODULE_LICENSE("GPL");
MODULE_AUTHOR("VEDACRAFT_NAM");
MODULE_DESCRIPTION("Conveyor Control Driver");

#define DEVICE_NAME "conveyor_mqtt"
#define BUF_LEN 256

#define GPIO_BASE 512
#define STEP_PIN (GPIO_BASE + 17)   // BCM 17
#define DIR_PIN (GPIO_BASE + 27)    // BCM 27  
#define ENABLE_PIN (GPIO_BASE + 22) // BCM 22
#define SERVO_PIN (GPIO_BASE + 5)  // BCM 5

// Character Device 관련
static int major_num; 
static char command_buffer[BUF_LEN];

// 모터 상태 관련
static int current_servo_angle = 0;    // 초기값 90도
static bool conveyor_running = false;      // 초기값 false
static bool step_direction = true;        // true=정방향, false=역방향

// 시스템 상태
static bool system_stop = false;           // 시스템 정지 신호

//타이머(에러)
static int current_speed = 100; //현재속도
static struct timer_list speed_timer; //속도 변경용 타이머

//스레드
static struct task_struct *stepper_thread = NULL;  
static struct task_struct *servo_thread = NULL; 
static bool threads_should_stop = false;  
static int target_servo_angle = 0;

// 스텝 카운터 관련 (수정!)
static int step_counter = 0;
static int current_panel = 0;
static bool auto_step_mode = false;

static bool error_mode = false;
static int error_step_count = 0;
static unsigned long error_start_time = 0;

// 다단계 판 간격 (새로 추가!)
static int first_panel_steps = 288;    // 첫 번째 칸: 6.5cm
static int normal_panel_steps = 392;   // 나머지 칸: 8.5cm
static int total_panels = 10;          // 총 10칸      // 자동 스텝 모드
static int target_steps_per_panel = 288; // 초기값: 6.5cm로 시작

static bool servo_auto_return = false;
static unsigned long push_start_time = 0;


//error_mode 시작 → 30초 정상 작동
//30초 후 → 1씩 3번 감소 (예: 30→29→28→27)
//3번 감소 후 → 그 속도로 계속 유지
static void speed_timer_callback(struct timer_list *t) {
    if (error_mode) {
        unsigned long elapsed_seconds = (jiffies - error_start_time) / HZ;
        printk(KERN_INFO "에러 모드 타이머 체크 - 경과시간: %lu초, 감소횟수: %d\n", 
               elapsed_seconds, error_step_count);
        
        // 30초가 지났고, 아직 2번 감소하지 않았다면
        if (elapsed_seconds >= 30 && error_step_count < 2) {
            // 1초마다 10씩 감소 (30초, 31초에 각각 감소)
            if (elapsed_seconds == 30 + error_step_count) {                
                current_speed -= 10;
                error_step_count++;
                printk(KERN_INFO "에러 모드: 속도 감소 %d번째 (현재 속도: %d)\n", 
                       error_step_count, current_speed);
            }
        }

        // 2번 감소 완료했는지 체크
        if (error_step_count >= 2) {
            error_mode = false;
            printk(KERN_INFO "에러 모드 완료 - 속도 %d로 고정 (2번 감소 완료)\n", current_speed);
            return; // 타이머 완전 종료
        }
            
        // 에러 모드가 계속 진행 중이면 타이머 재설정
        mod_timer(&speed_timer, jiffies + msecs_to_jiffies(1000));
        return; // 일반 속도 조절 로직 실행하지 않음
    }

}


 static int stepper_thread_func(void *data) {
    unsigned long delay_us;
    
    printk(KERN_INFO "스테퍼 모터 스레드 시작\n");
    
    while (!threads_should_stop && !kthread_should_stop()) {
        // 컨베이어가 동작 중일 때만 펄스 생성
        if (conveyor_running && !system_stop) {
            // 방향 설정
            gpio_set_value(DIR_PIN, step_direction ? 1 : 0);
            
            // 모터 활성화
            gpio_set_value(ENABLE_PIN, 0);  // LOW = 활성화
            
            // 스텝 펄스 생성
            gpio_set_value(STEP_PIN, 1);    // HIGH
            udelay(1000);                   // 1ms 펄스 폭 (10에서 1000으로 변경!)
            gpio_set_value(STEP_PIN, 0);    // LOW
            
            // 스텝 카운터 증가 (새로 추가!)
            step_counter++;
            
           if (auto_step_mode && step_counter >= target_steps_per_panel) {
            current_panel++;
            printk(KERN_INFO "판 %d 도달 완료 (%d 스텝)\n", 
                current_panel, step_counter);
            step_counter = 0;
            
            //서보 자동 동작 추가
            target_servo_angle = 180;  // 밀어내기
            servo_auto_return = true;
            push_start_time = jiffies;
            printk(KERN_INFO "자동 서보 밀어내기 실행\n");
            
            // 다음 판의 거리 설정
            if (current_panel == 1) {
                target_steps_per_panel = normal_panel_steps;
                printk(KERN_INFO "2번째 판부터 445 스텝 (8.5cm)으로 변경\n");
            }
            
            // 10칸 완료 시 처음으로 리셋
            if (current_panel >= total_panels) {
                current_panel = 0;
                target_steps_per_panel = first_panel_steps;
                printk(KERN_INFO "10칸 완료! 처음으로 리셋 - 첫 번째 칸 340 스텝 (6.5cm)\n");
            }
            
            // 잠시 멈춤 (판 교체 시간)
            msleep(1000);
            printk(KERN_INFO "다음 판으로 자동 이동 시작\n");
        }
            
            // 속도에 따른 딜레이 계산 (속도가 높을수록 딜레이 짧음)
            delay_us = 200000 / current_speed;  // 마이크로초 단위
            if (delay_us > 10000) delay_us = 10000;  // 최대 딜레이 제한
            if (delay_us < 100) delay_us = 100;      // 최소 딜레이 제한
            
            udelay(delay_us);
        } else {
            // 컨베이어 정지 시 모터 비활성화
            gpio_set_value(ENABLE_PIN, 1);  // HIGH = 비활성화
            gpio_set_value(STEP_PIN, 0);    // LOW
            
            // CPU 사용률 줄이기 위해 잠시 대기
            msleep(50);
        }
        
        // 스케줄러에게 CPU 양보
        schedule();
    }
    
    printk(KERN_INFO "스테퍼 모터 스레드 종료\n");
    return 0;
}
static int servo_thread_func(void *data) {
    int pulse_us;
    
    printk(KERN_INFO "서보 모터 스레드 시작\n");
    
    while (!threads_should_stop && !kthread_should_stop()) {
        // 자동 복귀 체크
        if (servo_auto_return && time_after(jiffies, push_start_time + HZ)) {
            target_servo_angle = 0;
            servo_auto_return = false;
            printk(KERN_INFO "서보 모터 자동 복귀(0도)\n");
        }
        
        // 각도 변경
        if (current_servo_angle != target_servo_angle) {
            current_servo_angle = target_servo_angle;
            printk(KERN_INFO "서보 각도 변경: %d도\n", current_servo_angle);
        }
        
        // wiringPi와 동일한 PWM 신호 (20ms 주기)
        pulse_us = 1000 + (current_servo_angle * 1000) / 180;  // 1000~2000us
        
        gpio_set_value(SERVO_PIN, 1);
        udelay(pulse_us);                    // 1~2ms HIGH
        gpio_set_value(SERVO_PIN, 0);
        udelay(20000 - pulse_us);            // 나머지 시간 (20ms 주기)
    }
    
    printk(KERN_INFO "서보 모터 스레드 종료\n");
    return 0;
}

static ssize_t conveyor_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    //버퍼 크기 검사
    if (len > BUF_LEN - 1)
        return -EINVAL;

    //사용자 공간에서 커널공간으로 데이터 복사
    if (copy_from_user(command_buffer, buf, len))
        return -EFAULT;

    //문자열 종료 문자 추가
    command_buffer[len] = '\0';

    //컨베이어 시작
    if (strncmp(command_buffer, "on", 2) == 0) {
        auto_step_mode = true;
        step_counter = 0;
        current_panel = 0;
        target_steps_per_panel = first_panel_steps;  // 6.5cm로 시작 (340스텝)
        conveyor_running = true;

        current_speed = 100;
        printk(KERN_INFO "컨베이어 시작 (첫 번째 판: 340 스텝)\n");
    }
    //컨베이어 종료
    else if (strncmp(command_buffer, "off", 3) == 0) {
        conveyor_running = false;
        auto_step_mode = false;
    }
    else if (strncmp(command_buffer, "error_mode", 10) == 0) {
        del_timer_sync(&speed_timer);

        // 컨베이어 자동 시작 (추가!)
        auto_step_mode = true;
        step_counter = 0;
        current_panel = 0;
        target_steps_per_panel = first_panel_steps;
        conveyor_running = true;

        error_mode = true;
        error_step_count = 0;
        error_start_time = jiffies;

        // 속도 초기화 (추가!)
        current_speed = 100;

        printk(KERN_INFO "에러 모드 시작 - 30초 후 속도 감소\n");
        printk(KERN_INFO "시작 시간: %lu\n", error_start_time);
        printk(KERN_INFO "현재 속도: %d\n", current_speed);
        printk(KERN_INFO "속도 감소 시작\n");
        mod_timer(&speed_timer, jiffies + msecs_to_jiffies(1000));
    }

    return len;

    }

static ssize_t conveyor_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    char status[512];
    int status_len;
    
    // 중복 읽기 방지
    if (*offset > 0)
        return 0;
    
    status_len = snprintf(status, sizeof(status),
        "=== 컨베이어 상태 ===\n"
         "컨베이어: %s\n"
         "서보 각도: %d도 (목표: %d도)\n"   
         "스테퍼 방향: %s\n"
         "현재 속도: %d\n"       
         "스레드 상태: %s\n"             
         "스텝 모드: %s\n"
        "현재 스텝: %d / %d\n"  
        "현재 판: %d\n",
         conveyor_running ? "ON" : "OFF",
         current_servo_angle, target_servo_angle, 
         step_direction ? "정방향" : "역방향",
         current_speed,            
         (stepper_thread && servo_thread) ? "동작중" : "정지",
          auto_step_mode ? "자동" : "수동",
        step_counter, target_steps_per_panel,
        current_panel);  // ← 스레드 상태 추가

    if (len < status_len)
        return -EINVAL;

    //커널 공간에서 사용자 공간으로 데이터 복사
    if (copy_to_user(buf, status, status_len))
        return -EFAULT;

    //중복읽기 방지
    *offset = status_len;

    // 4. 읽은 바이트 수 반환
    return status_len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = conveyor_write,
    .read = conveyor_read,
};

static int __init conveyor_init(void) {
    int ret;
    
    printk(KERN_INFO "컨베이어 드라이버 로드 시작\n");
    
    // 1. Character device 등록
    major_num = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_num < 0) {
        printk(KERN_ERR "Character device 등록 실패\n");
        return major_num;
    }
    //GPIO 핀 요청 및 오류 처리
    ret = gpio_request(STEP_PIN, "step_pin");
    if(ret){
        printk(KERN_ERR "STEP_PIN 요청 실패\n");
        unregister_chrdev(major_num, DEVICE_NAME);
        return ret;

    }

    ret = gpio_request(DIR_PIN, "dir_pin");
    if(ret){
        printk(KERN_ERR "DIR_PIN 요청 실패\n");
        gpio_free(STEP_PIN);
        unregister_chrdev(major_num, DEVICE_NAME);
        return ret;

    }

    ret = gpio_request(ENABLE_PIN, "enable_pin");
    if(ret){
        printk(KERN_ERR "ENABLE_PIN 요청 실패\n");
        gpio_free(STEP_PIN);
        gpio_free(DIR_PIN);
        unregister_chrdev(major_num, DEVICE_NAME);
        return ret;

    }

    ret = gpio_request(SERVO_PIN, "servo_pin");
    if(ret){
        printk(KERN_ERR "SERVO_PIN 요청 실패\n");
        gpio_free(STEP_PIN);
        gpio_free(DIR_PIN);
        gpio_free(ENABLE_PIN);
        unregister_chrdev(major_num, DEVICE_NAME);
        return ret;

    }

    //GPIO 핀들을 출력 모드로 설정
    // 3. GPIO 핀들을 출력 모드로 설정
ret = gpio_direction_output(STEP_PIN, 0);
ret = gpio_direction_output(DIR_PIN, 1);
ret = gpio_direction_output(ENABLE_PIN, 1);
ret = gpio_direction_output(SERVO_PIN, 0);
timer_setup(&speed_timer, speed_timer_callback, 0);

// 4. 스레드 생성 및 시작 ← 이 부분 전체 추가
threads_should_stop = false;

stepper_thread = kthread_run(stepper_thread_func, NULL, "stepper_motor");
if (IS_ERR(stepper_thread)) {
    printk(KERN_ERR "스테퍼 모터 스레드 생성 실패\n");
    ret = PTR_ERR(stepper_thread);
    goto cleanup_gpio;
}

servo_thread = kthread_run(servo_thread_func, NULL, "servo_motor");
if (IS_ERR(servo_thread)) {
    printk(KERN_ERR "서보 모터 스레드 생성 실패\n");
    ret = PTR_ERR(servo_thread);
    kthread_stop(stepper_thread);
    goto cleanup_gpio;
}

printk(KERN_INFO "컨베이어 드라이버 로드 완료 (스레드 2개 동작중)\n");
return 0;

cleanup_gpio: // ← 에러 처리 레이블 추가
    gpio_free(STEP_PIN);
    gpio_free(DIR_PIN);
    gpio_free(ENABLE_PIN);
    gpio_free(SERVO_PIN);
    unregister_chrdev(major_num, DEVICE_NAME);
    return ret;
}

static void __exit conveyor_exit(void) {
    threads_should_stop = true;
    conveyor_running = false;
    error_mode = false;
    
    // 서보를 0도로 복귀시키고 잠시 대기
    target_servo_angle = 0;
    msleep(500);  // 0.5초 대기해서 서보가 0도로 이동

    del_timer_sync(&speed_timer);
    
    if (stepper_thread) {
        kthread_stop(stepper_thread);
        printk(KERN_INFO "스테퍼 모터 스레드 종료 완료\n");
        stepper_thread = NULL;  // 추가: NULL로 설정
    }
    
    if (servo_thread) {
        kthread_stop(servo_thread);
        printk(KERN_INFO "서보 모터 스레드 종료 완료\n");
        servo_thread = NULL;  // 추가: NULL로 설정
    }

    //GPIO 핀들을 안전한 상태로
    gpio_set_value(STEP_PIN, 0);    // LOW
    gpio_set_value(DIR_PIN, 1);     // HIGH
    gpio_set_value(ENABLE_PIN, 1);  // HIGH (모터 비활성화)
    gpio_set_value(SERVO_PIN, 0);   // LOW
    
    //핀 해제
    gpio_free(STEP_PIN);
    gpio_free(DIR_PIN);
    gpio_free(ENABLE_PIN);
    gpio_free(SERVO_PIN);
    
    unregister_chrdev(major_num, DEVICE_NAME);
    
    printk(KERN_INFO "컨베이어 드라이버 언로드 완료\n");
}
module_init(conveyor_init);
module_exit(conveyor_exit);