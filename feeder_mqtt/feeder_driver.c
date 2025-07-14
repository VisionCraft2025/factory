#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/kthread.h> //커널 스레드
#include <linux/delay.h>
#include <linux/jiffies.h>



MODULE_LICENSE("GPL");
MODULE_AUTHOR("VisionCraft_min");
MODULE_DESCRIPTION("Stepper Motor Driver for Feeder");

#define DEVICE_NAME "feeder_01" //디바이스명
#define BUF_LEN 16 //명령어버퍼
#define NUM_PINS 4 //스텝모터 핀 4개임
#define MOTOR_DELAY_US 700 // 모터 속도

//static 붙여야 다른 모듈이랑 이름 같아도 충돌 안남

//static int pins[NUM_PINS] = {14, 15, 16, 17}; 
//static int pins[NUM_PINS] = {5, 6, 13, 19};
#define GPIO_BASE 512 // 커널에서는 GPIO번호에 +512 해야 동작함, gpiochip


static bool error_mode = false; // 에러모드(모터 속도 느려지는 거)

static int pins[NUM_PINS] = {
    GPIO_BASE + 5,
    GPIO_BASE + 6,
    GPIO_BASE + 13,
    GPIO_BASE + 19
};

// 스텝모터 회전하는 거
static int step_sequence[8][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

static struct task_struct *motor_thread; // 모터 회전용
static int direction = 1;  // 1: 정방향, -1: 역방향
static bool running = false; //회전중인지

static int major_num; //디바이스 major번호
static char command_buffer[BUF_LEN]; //명령어 받아서 제어

//모터 회전
static int motor_func(void *data) {
    int step = 0;
    unsigned long error_start_jiffies = 0;
    int slow_delay = MOTOR_DELAY_US;

    while (!kthread_should_stop()) {
        if (running) {
            int idx = step % 8;
            for (int i = 0; i < 4; i++)
                gpio_set_value(pins[i], step_sequence[idx][i]);
            step += direction;
            if (step < 0) step += 8;

            if (error_mode) {
                if (error_start_jiffies == 0)
                    error_start_jiffies = jiffies; // 에러모드 진입 시 시간 기록

                unsigned long elapsed_jiffies = jiffies - error_start_jiffies;
                unsigned long elapsed_ms = jiffies_to_msecs(elapsed_jiffies);

                if (elapsed_ms < 30000) {
                    // 30초까지는 기본 속도
                    slow_delay = MOTOR_DELAY_US;
                } else {
                    // 30초 이후부터 점차 느려짐
                    int max_delay = MOTOR_DELAY_US * 3;
                    int added_delay = ((elapsed_ms - 30000) / 1000) * 50;  // 초당 50us 증가
                    if (added_delay > (max_delay - MOTOR_DELAY_US))
                        added_delay = max_delay - MOTOR_DELAY_US;
                    slow_delay = MOTOR_DELAY_US + added_delay;
                }

                usleep_range(slow_delay, slow_delay + 200);
            } else {
                error_start_jiffies = 0;  // 에러모드 해제 시 초기화
                usleep_range(MOTOR_DELAY_US, MOTOR_DELAY_US + 200);
            }

        } else {
            msleep(50); //대기
        }
    }
    return 0;
}


// echo 명령어 받는 부분
static ssize_t feeder_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    if (len > BUF_LEN - 1)
        return -EINVAL;

    if (copy_from_user(command_buffer, buf, len))
        return -EFAULT;

    command_buffer[len] = '\0';

    if (strncmp(command_buffer, "on", 2) == 0) { //켜고
        direction = -1;
        running = true;
    } else if (strncmp(command_buffer, "reverse", 7) == 0) { //역방향 회전
        direction = 1;
        running = true;
    } else if (strncmp(command_buffer, "off", 3) == 0) { //끄기
        running = false;
        for (int i = 0; i < 4; i++)
            gpio_set_value(pins[i], 0);
    } else if (strncmp(command_buffer, "error", 5) == 0) {  //에러 모드
        error_mode = true;
        running = true;
    } else if (strncmp(command_buffer, "normal", 6) == 0) {  // 속도 복구 모드
        error_mode = false;
        running = true;
    }else {
        printk(KERN_INFO "Unknown command: %s\n", command_buffer);
    }

    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = feeder_write,
};

// insmod 초기화
static int __init feeder_init(void) {
    int ret;
    //gpio핀 제대로 꽂았는 지 확인
    printk(KERN_INFO "GPIO pins: %d %d %d %d\n", pins[0], pins[1], pins[2], pins[3]);

    //디ㅏ바이스 등록
    major_num = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_num < 0) {
        printk(KERN_ALERT "Feeder driver registration failed\n");
        return major_num;
    }

    //gpio 등록 되는지 체크
    for (int i = 0; i < NUM_PINS; i++) {
        ret = gpio_request(pins[i], "feeder_pin");
        if (ret) {
            printk(KERN_ALERT "Failed to request GPIO %d\n", pins[i]);
            return ret;
        }
        gpio_direction_output(pins[i], 0);
    }

// 모터 회전
    motor_thread = kthread_run(motor_func, NULL, "feeder_motor_thread");
    if (IS_ERR(motor_thread)) {
        printk(KERN_ALERT "Failed to create motor thread\n");
        return PTR_ERR(motor_thread);
    }

    printk(KERN_INFO "Feeder driver loaded. Major: %d\n", major_num);
    return 0;
}

//rmmod
static void __exit feeder_exit(void) {
    running = false;
    if (motor_thread)
        kthread_stop(motor_thread);

    for (int i = 0; i < NUM_PINS; i++)
        gpio_free(pins[i]);

    unregister_chrdev(major_num, DEVICE_NAME);
    printk(KERN_INFO "Feeder driver unloaded\n");
}


module_init(feeder_init);
module_exit(feeder_exit);
