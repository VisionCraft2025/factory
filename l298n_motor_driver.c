/*
 * L298N 모터 드라이버 커널 모듈
 * 라즈베리파이용 듀얼 DC 모터 제어 드라이버
 * 
 * 작성자: Your Name
 * 버전: 1.0
 * 라이센스: GPL
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/spinlock.h>

#define DEVICE_NAME "l298n_motor"
#define CLASS_NAME "motor"

/* PWM 제어 사용 여부 설정 */
#define USE_PWM_CONTROL 1  // 1: 점퍼 제거, PWM 사용 / 0: 점퍼 유지, 고정속도

/* L298N GPIO 핀 정의 (BCM 번호) */
#define GPIO_BASE      512  // 라즈베리파이 GPIO 기본 번호

#define MOTOR_A_IN1    (GPIO_BASE + 23)   // 모터A 방향 제어 1 (GPIO 23)
#define MOTOR_A_IN2    (GPIO_BASE + 24)   // 모터A 방향 제어 2 (GPIO 24)
#define MOTOR_B_IN3    (GPIO_BASE + 25)   // 모터B 방향 제어 1 (GPIO 25)
#define MOTOR_B_IN4    (GPIO_BASE + 8)    // 모터B 방향 제어 2 (GPIO 8)

#if USE_PWM_CONTROL
#define MOTOR_A_ENA    (GPIO_BASE + 18)   // 모터A 활성화/속도 제어 (GPIO 18)
#define MOTOR_B_ENB    (GPIO_BASE + 7)    // 모터B 활성화/속도 제어 (GPIO 7)
#endif

/* PWM 제어 구조체 */
struct pwm_control {
    struct hrtimer timer;       // 고해상도 타이머 사용
    int pin;
    int duty_cycle;             // 0-100 (%)
    int period_ns;              // PWM 주기 (나노초)
    int high_time_ns;           // HIGH 시간 (나노초)
    int is_high;                // 현재 상태
    int enabled;                // PWM 활성화 여부
    spinlock_t lock;            // 스핀락
};

/* PWM 제어 변수 */
static struct pwm_control pwm_a;
static struct pwm_control pwm_b;
static int major_number;
static struct class* motor_class = NULL;
static struct device* motor_device = NULL;

/* 함수 선언 */
static int motor_open(struct inode*, struct file*);
static int motor_release(struct inode*, struct file*);
static ssize_t motor_write(struct file*, const char*, size_t, loff_t*);
static ssize_t motor_read(struct file*, char*, size_t, loff_t*);

/* 파일 연산 구조체 */
static struct file_operations fops = {
    .open = motor_open,
    .read = motor_read,
    .write = motor_write,
    .release = motor_release,
};

/* PWM 고해상도 타이머 콜백 함수 */
static enum hrtimer_restart pwm_timer_callback(struct hrtimer *timer) {
    struct pwm_control *pwm = container_of(timer, struct pwm_control, timer);
    unsigned long flags;
    ktime_t next_time;
    
    spin_lock_irqsave(&pwm->lock, flags);
    
    if (!pwm->enabled) {
        spin_unlock_irqrestore(&pwm->lock, flags);
        return HRTIMER_NORESTART;
    }
    
    if (pwm->is_high) {
        // HIGH에서 LOW로 변경
        gpio_set_value(pwm->pin, 0);
        pwm->is_high = 0;
        
        // LOW 시간 설정
        int low_time_ns = pwm->period_ns - pwm->high_time_ns;
        if (low_time_ns > 0) {
            next_time = ktime_set(0, low_time_ns);
        } else {
            // duty cycle이 100%인 경우
            next_time = ktime_set(0, pwm->period_ns);
        }
    } else {
        // LOW에서 HIGH로 변경
        if (pwm->high_time_ns > 0) {
            gpio_set_value(pwm->pin, 1);
            pwm->is_high = 1;
            next_time = ktime_set(0, pwm->high_time_ns);
        } else {
            // duty cycle이 0%인 경우
            pwm->is_high = 0;
            next_time = ktime_set(0, pwm->period_ns);
        }
    }
    
    hrtimer_forward_now(&pwm->timer, next_time);
    spin_unlock_irqrestore(&pwm->lock, flags);
    
    return HRTIMER_RESTART;
}

/* PWM 초기화 */
static void pwm_init(struct pwm_control *pwm, int pin) {
    pwm->pin = pin;
    pwm->duty_cycle = 0;
    pwm->period_ns = 1000000;  // 1ms (1kHz) - 나노초 단위
    pwm->high_time_ns = 0;
    pwm->is_high = 0;
    pwm->enabled = 0;
    
    spin_lock_init(&pwm->lock);
    hrtimer_init(&pwm->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    pwm->timer.function = pwm_timer_callback;
    
    printk(KERN_INFO "L298N: PWM 초기화 - GPIO %d, 주기 1ms\n", pin);
}

/* PWM 설정 */
static void pwm_set_duty_cycle(struct pwm_control *pwm, int duty_percent) {
    unsigned long flags;
    
    if (duty_percent < 0) duty_percent = 0;
    if (duty_percent > 100) duty_percent = 100;
    
    spin_lock_irqsave(&pwm->lock, flags);
    
    pwm->duty_cycle = duty_percent;
    pwm->high_time_ns = (pwm->period_ns * duty_percent) / 100;
    
    printk(KERN_INFO "L298N: PWM 설정 - GPIO %d, Duty: %d%%, High: %dns\n", 
           pwm->pin, duty_percent, pwm->high_time_ns);
    
    if (duty_percent == 0) {
        if (pwm->enabled) {
            hrtimer_cancel(&pwm->timer);
            pwm->enabled = 0;
        }
        gpio_set_value(pwm->pin, 0);
    } else {
        if (!pwm->enabled) {
            pwm->enabled = 1;
            pwm->is_high = 0;
            // 즉시 시작
            hrtimer_start(&pwm->timer, ktime_set(0, 1000), HRTIMER_MODE_REL);
        }
    }
    
    spin_unlock_irqrestore(&pwm->lock, flags);
}

/* PWM 정지 */
static void pwm_stop(struct pwm_control *pwm) {
    unsigned long flags;
    
    spin_lock_irqsave(&pwm->lock, flags);
    pwm->enabled = 0;
    spin_unlock_irqrestore(&pwm->lock, flags);
    
    hrtimer_cancel(&pwm->timer);
    gpio_set_value(pwm->pin, 0);
    
    printk(KERN_INFO "L298N: PWM 정지 - GPIO %d\n", pwm->pin);
}
/* 모터 제어 함수들 */
static void motor_a_control(int direction, int speed) {
    printk(KERN_INFO "L298N: 모터A 제어 - 방향: %d, 속도: %d\n", direction, speed);
    
    /* 방향 제어 */
    switch(direction) {
        case 1:  // 정방향
            gpio_set_value(MOTOR_A_IN1, 1);
            gpio_set_value(MOTOR_A_IN2, 0);
            break;
            
        case -1: // 역방향
            gpio_set_value(MOTOR_A_IN1, 0);
            gpio_set_value(MOTOR_A_IN2, 1);
            break;
            
        case 0:  // 브레이크
            gpio_set_value(MOTOR_A_IN1, 1);
            gpio_set_value(MOTOR_A_IN2, 1);
            break;
            
        default: // 프리휠링
            gpio_set_value(MOTOR_A_IN1, 0);
            gpio_set_value(MOTOR_A_IN2, 0);
            break;
    }
    
#if USE_PWM_CONTROL
    /* 속도 제어 (소프트웨어 PWM) */
    if (direction != 0 && speed > 0) {
        pwm_set_duty_cycle(&pwm_a, speed);
    } else {
        pwm_stop(&pwm_a);
    }
#endif
}

static void motor_b_control(int direction, int speed) {
    printk(KERN_INFO "L298N: 모터B 제어 - 방향: %d, 속도: %d\n", direction, speed);
    
    /* 방향 제어 */
    switch(direction) {
        case 1:  // 정방향
            gpio_set_value(MOTOR_B_IN3, 1);
            gpio_set_value(MOTOR_B_IN4, 0);
            break;
            
        case -1: // 역방향
            gpio_set_value(MOTOR_B_IN3, 0);
            gpio_set_value(MOTOR_B_IN4, 1);
            break;
            
        case 0:  // 브레이크
            gpio_set_value(MOTOR_B_IN3, 1);
            gpio_set_value(MOTOR_B_IN4, 1);
            break;
            
        default: // 프리휠링
            gpio_set_value(MOTOR_B_IN3, 0);
            gpio_set_value(MOTOR_B_IN4, 0);
            break;
    }
    
#if USE_PWM_CONTROL
    /* 속도 제어 (소프트웨어 PWM) */
    if (direction != 0 && speed > 0) {
        pwm_set_duty_cycle(&pwm_b, speed);
    } else {
        pwm_stop(&pwm_b);
    }
#endif
}

static void motor_stop_all(void) {
    printk(KERN_INFO "L298N: 모든 모터 정지\n");
    motor_a_control(0, 0);
    motor_b_control(0, 0);
    
#if USE_PWM_CONTROL
    pwm_stop(&pwm_a);
    pwm_stop(&pwm_b);
#endif
}

/* 모듈 초기화 함수 */
static int __init l298n_init(void) {
    int result = 0;
    
    printk(KERN_INFO "L298N Driver: 모듈 초기화 시작\n");
    
    /* GPIO 핀 요청 */
    if ((result = gpio_request(MOTOR_A_IN1, "motor_a_in1")) < 0) {
        printk(KERN_ERR "L298N Driver: GPIO %d 요청 실패\n", MOTOR_A_IN1);
        return result;
    }
    
    if ((result = gpio_request(MOTOR_A_IN2, "motor_a_in2")) < 0) {
        printk(KERN_ERR "L298N Driver: GPIO %d 요청 실패\n", MOTOR_A_IN2);
        goto cleanup_in1;
    }
    
    if ((result = gpio_request(MOTOR_B_IN3, "motor_b_in3")) < 0) {
        printk(KERN_ERR "L298N Driver: GPIO %d 요청 실패\n", MOTOR_B_IN3);
        goto cleanup_in2;
    }
    
    if ((result = gpio_request(MOTOR_B_IN4, "motor_b_in4")) < 0) {
        printk(KERN_ERR "L298N Driver: GPIO %d 요청 실패\n", MOTOR_B_IN4);
        goto cleanup_in3;
    }

#if USE_PWM_CONTROL
    if ((result = gpio_request(MOTOR_A_ENA, "motor_a_ena")) < 0) {
        printk(KERN_ERR "L298N Driver: GPIO %d 요청 실패\n", MOTOR_A_ENA);
        goto cleanup_in4;
    }
    
    if ((result = gpio_request(MOTOR_B_ENB, "motor_b_enb")) < 0) {
        printk(KERN_ERR "L298N Driver: GPIO %d 요청 실패\n", MOTOR_B_ENB);
        goto cleanup_ena;
    }
    
    /* ENA, ENB를 출력으로 강제 설정 */
    gpio_direction_output(MOTOR_A_ENA, 0);
    gpio_direction_output(MOTOR_B_ENB, 0);
    printk(KERN_INFO "L298N Driver: GPIO %d, %d를 출력 모드로 설정\n", MOTOR_A_ENA, MOTOR_B_ENB);
#endif
    
    /* GPIO 출력 모드 설정 및 초기화 */
    gpio_direction_output(MOTOR_A_IN1, 0);
    gpio_direction_output(MOTOR_A_IN2, 0);
    gpio_direction_output(MOTOR_B_IN3, 0);
    gpio_direction_output(MOTOR_B_IN4, 0);
    
    printk(KERN_INFO "L298N Driver: 방향 제어 핀들을 출력 모드로 설정\n");
    
#if USE_PWM_CONTROL
    /* PWM 초기화 */
    pwm_init(&pwm_a, MOTOR_A_ENA);
    pwm_init(&pwm_b, MOTOR_B_ENB);
    printk(KERN_INFO "L298N Driver: 소프트웨어 PWM 모드로 설정됨\n");
#else
    printk(KERN_INFO "L298N Driver: 고정 속도 모드로 설정됨 (점퍼 사용)\n");
#endif
    
    /* 캐릭터 디바이스 등록 */
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ERR "L298N Driver: 메이저 번호 할당 실패 (%d)\n", major_number);
        result = major_number;
        goto cleanup_gpio;
    }
    
    /* 디바이스 클래스 생성 */
    motor_class = class_create(CLASS_NAME);
    if (IS_ERR(motor_class)) {
        printk(KERN_ERR "L298N Driver: 클래스 생성 실패\n");
        result = PTR_ERR(motor_class);
        goto cleanup_chrdev;
    }
    
    /* 디바이스 생성 */
    motor_device = device_create(motor_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(motor_device)) {
        printk(KERN_ERR "L298N Driver: 디바이스 생성 실패\n");
        result = PTR_ERR(motor_device);
        goto cleanup_class;
    }
    
    printk(KERN_INFO "L298N Driver: 초기화 완료 (메이저 번호: %d)\n", major_number);
    printk(KERN_INFO "L298N Driver: 디바이스 파일: /dev/%s\n", DEVICE_NAME);
    
    return 0;
    
cleanup_class:
    class_destroy(motor_class);
cleanup_chrdev:
    unregister_chrdev(major_number, DEVICE_NAME);
cleanup_gpio:
#if USE_PWM_CONTROL
    gpio_free(MOTOR_B_ENB);
cleanup_ena:
    gpio_free(MOTOR_A_ENA);
cleanup_in4:
#endif
    gpio_free(MOTOR_B_IN4);
cleanup_in3:
    gpio_free(MOTOR_B_IN3);
cleanup_in2:
    gpio_free(MOTOR_A_IN2);
cleanup_in1:
    gpio_free(MOTOR_A_IN1);
    
    return result;
}

/* 모듈 종료 함수 */
static void __exit l298n_exit(void) {
    printk(KERN_INFO "L298N Driver: 모듈 종료 시작\n");
    
    /* 모든 모터 정지 및 PWM 정리 */
    motor_stop_all();
    
#if USE_PWM_CONTROL
    hrtimer_cancel(&pwm_a.timer);
    hrtimer_cancel(&pwm_b.timer);
#endif
    
    /* 디바이스 및 클래스 해제 */
    device_destroy(motor_class, MKDEV(major_number, 0));
    class_unregister(motor_class);
    class_destroy(motor_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    
    /* GPIO 핀 해제 */
    gpio_free(MOTOR_A_IN1);
    gpio_free(MOTOR_A_IN2);
    gpio_free(MOTOR_B_IN3);
    gpio_free(MOTOR_B_IN4);
    
#if USE_PWM_CONTROL
    gpio_free(MOTOR_A_ENA);
    gpio_free(MOTOR_B_ENB);
#endif
    
    printk(KERN_INFO "L298N Driver: 모듈 종료 완료\n");
}

/* 디바이스 파일 연산 함수들 */
static int motor_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "L298N Driver: 디바이스 열림\n");
    return 0;
}

static ssize_t motor_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    char status_msg[200];
    int msg_len;
    
    if (*offset > 0) {
        return 0;  // EOF
    }
    
    msg_len = snprintf(status_msg, sizeof(status_msg),
        "L298N Motor Driver Status\n"
        "PWM Control: %s\n"
        "Device: /dev/%s\n"
        "Commands: A/B <direction> <speed>\n"
        "Direction: 1(forward), -1(reverse), 0(stop)\n"
        "Speed: 0-100 (PWM mode only)\n",
        USE_PWM_CONTROL ? "Enabled" : "Disabled",
        DEVICE_NAME);
    
    if (len < msg_len) {
        return -EINVAL;
    }
    
    if (copy_to_user(buffer, status_msg, msg_len)) {
        return -EFAULT;
    }
    
    *offset = msg_len;
    return msg_len;
}

static ssize_t motor_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    char command[64];
    char motor;
    int direction, speed;
    int parsed;
    
    if (len > 63) len = 63;
    
    if (copy_from_user(command, buffer, len)) {
        return -EFAULT;
    }
    command[len] = '\0';
    
    /* 명령어 파싱 */
    parsed = sscanf(command, "%c %d %d", &motor, &direction, &speed);
    
    if (parsed < 2) {
        printk(KERN_ERR "L298N Driver: 잘못된 명령어 형식: %s\n", command);
        printk(KERN_INFO "L298N Driver: 사용법 - A/B <direction> [speed]\n");
        return -EINVAL;
    }
    
    if (parsed == 2) {
        speed = 100;  // 기본 속도
    }
    
    /* 입력 값 검증 */
    if (direction < -1 || direction > 1) {
        printk(KERN_ERR "L298N Driver: 잘못된 방향 값: %d (허용: -1, 0, 1)\n", direction);
        return -EINVAL;
    }
    
    if (speed < 0 || speed > 100) {
        printk(KERN_ERR "L298N Driver: 잘못된 속도 값: %d (허용: 0-100)\n", speed);
        return -EINVAL;
    }
    
    /* 모터 제어 */
    if (motor == 'A' || motor == 'a') {
        motor_a_control(direction, speed);
    } else if (motor == 'B' || motor == 'b') {
        motor_b_control(direction, speed);
    } else if (motor == 'S' || motor == 's') {
        motor_stop_all();
    } else {
        printk(KERN_ERR "L298N Driver: 잘못된 모터 지정: %c (허용: A, B, S)\n", motor);
        return -EINVAL;
    }
    
    return len;
}

static int motor_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "L298N Driver: 디바이스 닫힘\n");
    return 0;
}

/* 모듈 정보 */
module_init(l298n_init);
module_exit(l298n_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("라즈베리파이용 L298N 모터 드라이버 커널 모듈");
MODULE_VERSION("1.0");
MODULE_INFO(intree, "Y");