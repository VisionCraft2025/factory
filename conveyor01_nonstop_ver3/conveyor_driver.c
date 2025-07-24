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
static int current_servo_angle = 0;
static bool conveyor_running = false;
static bool step_direction = true;

// 시스템 상태
static bool system_stop = false;

// 타이머
static int current_speed = 20;
static struct timer_list speed_timer;

// 스레드 - 중요: 초기화와 NULL 체크 강화
static struct task_struct *stepper_thread = NULL;  
static struct task_struct *servo_thread = NULL; 
static bool threads_should_stop = false;  
static int target_servo_angle = 0;

// 스텝 카운터 관련
static int step_counter = 0;
static int current_panel = 0;
static bool auto_step_mode = false;

static bool error_mode = false;
static int error_step_count = 0;
static unsigned long error_start_time = 0;

// 다단계 판 간격
static int first_panel_steps = 288;
static int normal_panel_steps = 392;
static int total_panels = 10;
static int target_steps_per_panel = 288;

static bool servo_auto_return = false;
static unsigned long push_start_time = 0;

// 초기화 완료 플래그 추가
static bool driver_initialized = false;

static void speed_timer_callback(struct timer_list *t) {
    if (error_mode) {
        unsigned long elapsed_seconds = (jiffies - error_start_time) / HZ;
        printk(KERN_INFO "에러 모드 타이머 체크 - 경과시간: %lu초, 감소횟수: %d\n", 
               elapsed_seconds, error_step_count);
        
        if (elapsed_seconds >= 30 && error_step_count < 2) {
            if (elapsed_seconds == 30 + error_step_count) {                
                current_speed -= 10;
                error_step_count++;
                printk(KERN_INFO "에러 모드: 속도 감소 %d번째 (현재 속도: %d)\n", 
                       error_step_count, current_speed);
            }
        }

        if (error_step_count >= 2) {
            error_mode = false;
            printk(KERN_INFO "에러 모드 완료 - 속도 %d로 고정 (2번 감소 완료)\n", current_speed);
            return;
        }
            
        mod_timer(&speed_timer, jiffies + msecs_to_jiffies(1000));
        return;
    }
}

static int stepper_thread_func(void *data) {
    unsigned long delay_us;
    
    printk(KERN_INFO "스테퍼 모터 스레드 시작\n");
    
    while (!threads_should_stop && !kthread_should_stop()) {
        if (conveyor_running && !system_stop) {
            gpio_set_value(DIR_PIN, step_direction ? 1 : 0);
            gpio_set_value(ENABLE_PIN, 0);
            
            gpio_set_value(STEP_PIN, 1);
            udelay(1000);
            gpio_set_value(STEP_PIN, 0);
            
            step_counter++;
            
            delay_us = 300000 / current_speed;
            if (delay_us > 50000) delay_us = 30000;
            if (delay_us < 100) delay_us = 100;
            
            udelay(delay_us);
        } else {
            gpio_set_value(ENABLE_PIN, 1);
            gpio_set_value(STEP_PIN, 0);
            msleep(50);
        }
        
        schedule();
    }
    
    printk(KERN_INFO "스테퍼 모터 스레드 종료\n");
    return 0;
}

static int servo_thread_func(void *data) {
    int pulse_us;
    
    printk(KERN_INFO "서보 모터 스레드 시작\n");
    
    while (!threads_should_stop && !kthread_should_stop()) {
        if (servo_auto_return && time_after(jiffies, push_start_time + HZ)) {
            target_servo_angle = 0;
            servo_auto_return = false;
            printk(KERN_INFO "서보 모터 자동 복귀(0도)\n");
        }
        
        if (current_servo_angle != target_servo_angle) {
            current_servo_angle = target_servo_angle;
            printk(KERN_INFO "서보 각도 변경: %d도\n", current_servo_angle);
        }
        
        pulse_us = 1000 + (current_servo_angle * 1000) / 180;
        
        gpio_set_value(SERVO_PIN, 1);
        udelay(pulse_us);
        gpio_set_value(SERVO_PIN, 0);
        udelay(20000 - pulse_us);
    }
    
    printk(KERN_INFO "서보 모터 스레드 종료\n");
    return 0;
}

static ssize_t conveyor_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    if (len > BUF_LEN - 1)
        return -EINVAL;

    if (copy_from_user(command_buffer, buf, len))
        return -EFAULT;

    command_buffer[len] = '\0';

    if (strncmp(command_buffer, "on", 2) == 0) {
        auto_step_mode = true;
        step_counter = 0;
        current_panel = 0;
        target_steps_per_panel = first_panel_steps;
        conveyor_running = true;
        current_speed = 100;
        printk(KERN_INFO "컨베이어 시작 (첫 번째 판: 340 스텝)\n");
    }
    else if (strncmp(command_buffer, "off", 3) == 0) {
        conveyor_running = false;
        auto_step_mode = false;
    }
    else if (strncmp(command_buffer, "error_mode", 10) == 0) {
        del_timer_sync(&speed_timer);

        auto_step_mode = true;
        step_counter = 0;
        current_panel = 0;
        target_steps_per_panel = first_panel_steps;
        conveyor_running = true;

        error_mode = true;
        error_step_count = 0;
        error_start_time = jiffies;
        current_speed = 100;

        printk(KERN_INFO "에러 모드 시작 - 30초 후 속도 감소\n");
        mod_timer(&speed_timer, jiffies + msecs_to_jiffies(1000));
    }

    return len;
}

static ssize_t conveyor_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    char status[512];
    int status_len;
    
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
        current_panel);

    if (len < status_len)
        return -EINVAL;

    if (copy_to_user(buf, status, status_len))
        return -EFAULT;

    *offset = status_len;
    return status_len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = conveyor_write,
    .read = conveyor_read,
};

static int __init conveyor_init(void) {
    int ret;
    
    printk(KERN_INFO "=== 컨베이어 드라이버 안전 로드 시작 ===\n");
    
    // 초기화 플래그 설정
    driver_initialized = false;
    
    // 스레드 포인터 초기화
    stepper_thread = NULL;
    servo_thread = NULL;
    threads_should_stop = false;
    
    // Character device 등록
    major_num = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_num < 0) {
        printk(KERN_ERR "Character device 등록 실패\n");
        return major_num;
    }
    printk(KERN_INFO "Character device 등록 완료 (Major: %d)\n", major_num);
    
    // GPIO 핀 요청
    ret = gpio_request(STEP_PIN, "step_pin");
    if (ret) {
        printk(KERN_ERR "STEP_PIN 요청 실패\n");
        goto cleanup_chrdev;
    }

    ret = gpio_request(DIR_PIN, "dir_pin");
    if (ret) {
        printk(KERN_ERR "DIR_PIN 요청 실패\n");
        goto cleanup_step_pin;
    }

    ret = gpio_request(ENABLE_PIN, "enable_pin");
    if (ret) {
        printk(KERN_ERR "ENABLE_PIN 요청 실패\n");
        goto cleanup_dir_pin;
    }

    ret = gpio_request(SERVO_PIN, "servo_pin");
    if (ret) {
        printk(KERN_ERR "SERVO_PIN 요청 실패\n");
        goto cleanup_enable_pin;
    }
    
    printk(KERN_INFO "모든 GPIO 핀 요청 완료\n");

    // GPIO 핀 방향 설정
    ret = gpio_direction_output(STEP_PIN, 0);
    ret |= gpio_direction_output(DIR_PIN, 1);
    ret |= gpio_direction_output(ENABLE_PIN, 1);
    ret |= gpio_direction_output(SERVO_PIN, 0);
    
    if (ret) {
        printk(KERN_ERR "GPIO 방향 설정 실패\n");
        goto cleanup_servo_pin;
    }
    
    printk(KERN_INFO "GPIO 방향 설정 완료\n");
    
    // 타이머 설정
    timer_setup(&speed_timer, speed_timer_callback, 0);
    printk(KERN_INFO "타이머 설정 완료\n");

    // 스레드 생성 및 시작
    stepper_thread = kthread_run(stepper_thread_func, NULL, "stepper_motor");
    if (IS_ERR(stepper_thread)) {
        printk(KERN_ERR "스테퍼 모터 스레드 생성 실패\n");
        ret = PTR_ERR(stepper_thread);
        stepper_thread = NULL;  // 중요: 에러 시 NULL로 설정
        goto cleanup_timer;
    }
    printk(KERN_INFO "스테퍼 모터 스레드 생성 완료\n");

    servo_thread = kthread_run(servo_thread_func, NULL, "servo_motor");
    if (IS_ERR(servo_thread)) {
        printk(KERN_ERR "서보 모터 스레드 생성 실패\n");
        ret = PTR_ERR(servo_thread);
        servo_thread = NULL;  // 중요: 에러 시 NULL로 설정
        goto cleanup_stepper_thread;
    }
    printk(KERN_INFO "서보 모터 스레드 생성 완료\n");

    // 초기화 완료
    driver_initialized = true;
    printk(KERN_INFO "=== 컨베이어 드라이버 로드 완료 (안전 모드) ===\n");
    return 0;

// 에러 처리 순서 (역순으로 정리)
cleanup_stepper_thread:
    if (stepper_thread && !IS_ERR(stepper_thread)) {
        threads_should_stop = true;
        kthread_stop(stepper_thread);
        stepper_thread = NULL;
    }

cleanup_timer:
    del_timer_sync(&speed_timer);

cleanup_servo_pin:
    gpio_free(SERVO_PIN);

cleanup_enable_pin:
    gpio_free(ENABLE_PIN);

cleanup_dir_pin:
    gpio_free(DIR_PIN);

cleanup_step_pin:
    gpio_free(STEP_PIN);

cleanup_chrdev:
    unregister_chrdev(major_num, DEVICE_NAME);
    
    printk(KERN_ERR "=== 컨베이어 드라이버 로드 실패 ===\n");
    return ret;
}

static void __exit conveyor_exit(void) {
    printk(KERN_INFO "=== 안전한 컨베이어 드라이버 종료 시작 ===\n");
    
    // 1. 즉시 모든 동작 중단
    threads_should_stop = true;
    conveyor_running = false;
    error_mode = false;
    printk(KERN_INFO "1단계: 모든 동작 중단 신호 전송 완료\n");
    
    // 2. 타이머 정지 (스레드보다 먼저)
    del_timer_sync(&speed_timer);
    printk(KERN_INFO "2단계: 타이머 정지 완료\n");
    
    // 3. 스레드 안전 종료 - NULL 체크 강화!
    if (stepper_thread && !IS_ERR(stepper_thread)) {
        printk(KERN_INFO "3-1단계: 스테퍼 스레드 종료 시작...\n");
        int ret = kthread_stop(stepper_thread);
        if (ret == 0) {
            printk(KERN_INFO "3-1단계: 스테퍼 스레드 정상 종료\n");
        } else {
            printk(KERN_WARNING "3-1단계: 스테퍼 스레드 강제 종료 (ret: %d)\n", ret);
        }
        stepper_thread = NULL;
    } else {
        printk(KERN_INFO "3-1단계: 스테퍼 스레드가 이미 종료되었거나 생성되지 않음\n");
    }
    
    if (servo_thread && !IS_ERR(servo_thread)) {
        printk(KERN_INFO "3-2단계: 서보 스레드 종료 시작...\n");
        int ret = kthread_stop(servo_thread);
        if (ret == 0) {
            printk(KERN_INFO "3-2단계: 서보 스레드 정상 종료\n");
        } else {
            printk(KERN_WARNING "3-2단계: 서보 스레드 강제 종료 (ret: %d)\n", ret);
        }
        servo_thread = NULL;
    } else {
        printk(KERN_INFO "3-2단계: 서보 스레드가 이미 종료되었거나 생성되지 않음\n");
    }
    
    // 4. GPIO 안전 상태 설정
    printk(KERN_INFO "4단계: GPIO 안전 상태 설정 중...\n");
    gpio_set_value(STEP_PIN, 0);      // LOW
    gpio_set_value(DIR_PIN, 1);       // HIGH  
    gpio_set_value(ENABLE_PIN, 1);    // HIGH (모터 비활성화)
    gpio_set_value(SERVO_PIN, 0);     // LOW
    printk(KERN_INFO "4단계: GPIO 안전 상태 설정 완료\n");
    
    // 5. GPIO 해제 (순서 중요)
    printk(KERN_INFO "5단계: GPIO 해제 중...\n");
    gpio_free(SERVO_PIN);
    gpio_free(ENABLE_PIN);
    gpio_free(DIR_PIN);
    gpio_free(STEP_PIN);
    printk(KERN_INFO "5단계: GPIO 해제 완료\n");
    
    // 6. Character device 등록 해제
    unregister_chrdev(major_num, DEVICE_NAME);
    printk(KERN_INFO "6단계: 디바이스 등록 해제 완료\n");
    
    // 7. 초기화 플래그 리셋
    driver_initialized = false;
    
    printk(KERN_INFO "=== 컨베이어 드라이버 안전 종료 완료 ===\n");
}

module_init(conveyor_init);
module_exit(conveyor_exit);