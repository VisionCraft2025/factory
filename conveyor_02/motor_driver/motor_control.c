/*
 * L298N 모터 제어 터미널 인터페이스
 * 사용자 친화적인 모터 속도 제어 프로그램
 * 
 * 컴파일: gcc -o motor_control motor_control.c
 * 실행: ./motor_control
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>

#define DEVICE_PATH "/dev/l298n_motor"
#define MAX_COMMAND_LEN 50

// 전역 변수
int device_fd = -1;
int current_motor_a_speed = 0;
int current_motor_b_speed = 0;
int current_motor_a_dir = 0;   // -1: 역방향, 0: 정지, 1: 정방향
int current_motor_b_dir = 0;

// 색상 정의
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

// 함수 선언
void print_header(void);
void print_status(void);
void print_help(void);
void send_motor_command(char motor, int direction, int speed);
void cleanup_and_exit(int sig);
int open_device(void);
void close_device(void);
void clear_screen(void);
void motor_control_loop(void);
int validate_speed(int speed);

// 디바이스 열기
int open_device(void) {
    device_fd = open(DEVICE_PATH, O_WRONLY);
    if (device_fd < 0) {
        printf(COLOR_RED "오류: 디바이스 파일을 열 수 없습니다: %s\n" COLOR_RESET, DEVICE_PATH);
        printf("모듈이 로드되었는지 확인하세요: sudo insmod l298n_motor_driver.ko\n");
        return -1;
    }
    return 0;
}

// 디바이스 닫기
void close_device(void) {
    if (device_fd >= 0) {
        close(device_fd);
        device_fd = -1;
    }
}

// 화면 지우기
void clear_screen(void) {
    printf("\033[2J\033[H");
}

// 헤더 출력
void print_header(void) {
    printf(COLOR_BOLD COLOR_CYAN);
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    L298N 모터 제어 시스템                     ║\n");
    printf("║                    라즈베리파이 커널 드라이버                  ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);
}

// 현재 상태 출력
void print_status(void) {
    printf(COLOR_BOLD "\n현재 모터 상태:\n" COLOR_RESET);
    printf("┌─────────┬─────────┬─────────┬─────────────────┐\n");
    printf("│  모터   │  방향   │  속도   │      상태       │\n");
    printf("├─────────┼─────────┼─────────┼─────────────────┤\n");
    
    // 모터A 상태
    const char* dir_a = (current_motor_a_dir == 1) ? "정방향" : 
                       (current_motor_a_dir == -1) ? "역방향" : "정지";
    const char* color_a = (current_motor_a_dir == 0) ? COLOR_RED : COLOR_GREEN;
    
    printf("│    A    │  %s%6s%s  │  %s%3d%%%s  │", 
           color_a, dir_a, COLOR_RESET,
           color_a, current_motor_a_speed, COLOR_RESET);
    
    // 상태 바
    printf(" ");
    for (int i = 0; i < 10; i++) {
        if (i < current_motor_a_speed / 10) {
            printf(COLOR_GREEN "█" COLOR_RESET);
        } else {
            printf("░");
        }
    }
    printf(" │\n");
    
    // 모터B 상태
    const char* dir_b = (current_motor_b_dir == 1) ? "정방향" : 
                       (current_motor_b_dir == -1) ? "역방향" : "정지";
    const char* color_b = (current_motor_b_dir == 0) ? COLOR_RED : COLOR_GREEN;
    
    printf("│    B    │  %s%6s%s  │  %s%3d%%%s  │", 
           color_b, dir_b, COLOR_RESET,
           color_b, current_motor_b_speed, COLOR_RESET);
    
    // 상태 바
    printf(" ");
    for (int i = 0; i < 10; i++) {
        if (i < current_motor_b_speed / 10) {
            printf(COLOR_GREEN "█" COLOR_RESET);
        } else {
            printf("░");
        }
    }
    printf(" │\n");
    
    printf("└─────────┴─────────┴─────────┴─────────────────┘\n");
}

// 도움말 출력
void print_help(void) {
    printf(COLOR_YELLOW "\n사용법:\n" COLOR_RESET);
    printf("  a <속도>     - 모터A 정방향 (0-100)\n");
    printf("  -a <속도>    - 모터A 역방향 (0-100)\n");
    printf("  b <속도>     - 모터B 정방향 (0-100)\n");
    printf("  -b <속도>    - 모터B 역방향 (0-100)\n");
    printf("  stop         - 모든 모터 정지\n");
    printf("  status       - 현재 상태 표시\n");
    printf("  help         - 이 도움말 표시\n");
    printf("  clear        - 화면 지우기\n");
    printf("  quit         - 프로그램 종료\n");
    
    printf(COLOR_CYAN "\n빠른 명령어:\n" COLOR_RESET);
    printf("  0            - 모든 모터 정지\n");
    printf("  1-100        - 모터A 정방향 (해당 속도)\n");
    printf("  -1 ~ -100    - 모터A 역방향 (해당 속도)\n");
}

// 속도 유효성 검사
int validate_speed(int speed) {
    if (speed < 0 || speed > 100) {
        printf(COLOR_RED "오류: 속도는 0-100 범위여야 합니다.\n" COLOR_RESET);
        return 0;
    }
    return 1;
}

// 모터 명령 전송
void send_motor_command(char motor, int direction, int speed) {
    char command[MAX_COMMAND_LEN];
    snprintf(command, sizeof(command), "%c %d %d", motor, direction, speed);
    
    if (write(device_fd, command, strlen(command)) < 0) {
        printf(COLOR_RED "오류: 명령 전송 실패 - %s\n" COLOR_RESET, strerror(errno));
        return;
    }
    
    // 상태 업데이트
    if (motor == 'A' || motor == 'a') {
        current_motor_a_dir = direction;
        current_motor_a_speed = (direction == 0) ? 0 : speed;
    } else if (motor == 'B' || motor == 'b') {
        current_motor_b_dir = direction;
        current_motor_b_speed = (direction == 0) ? 0 : speed;
    } else if (motor == 'S' || motor == 's') {
        current_motor_a_dir = current_motor_b_dir = 0;
        current_motor_a_speed = current_motor_b_speed = 0;
    }
    
    printf(COLOR_GREEN "✓ 명령 전송 완료: %s\n" COLOR_RESET, command);
}

// 정리 및 종료
void cleanup_and_exit(int sig) {
    printf(COLOR_YELLOW "\n\n프로그램을 종료합니다...\n" COLOR_RESET);
    
    // 모든 모터 정지
    if (device_fd >= 0) {
        send_motor_command('S', 0, 0);
        printf(COLOR_GREEN "모든 모터를 정지했습니다.\n" COLOR_RESET);
    }
    
    close_device();
    printf(COLOR_CYAN "안전하게 종료되었습니다.\n" COLOR_RESET);
    exit(0);
}

// 메인 제어 루프
void motor_control_loop(void) {
    char input[100];
    char command[50];
    int value;
    
    while (1) {
        printf(COLOR_BOLD "\nL298N> " COLOR_RESET);
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        // 개행 문자 제거
        input[strcspn(input, "\n")] = 0;
        
        // 빈 입력 처리
        if (strlen(input) == 0) {
            continue;
        }
        
        // 단순 숫자 입력 처리 (빠른 모터A 제어)
        if (sscanf(input, "%d", &value) == 1) {
            if (value == 0) {
                send_motor_command('S', 0, 0);
            } else if (value > 0 && value <= 100) {
                send_motor_command('A', 1, value);
            } else if (value < 0 && value >= -100) {
                send_motor_command('A', -1, -value);
            } else {
                printf(COLOR_RED "오류: 유효하지 않은 속도 값입니다.\n" COLOR_RESET);
            }
            continue;
        }
        
        // 명령어 파싱
        if (sscanf(input, "%s %d", command, &value) == 2) {
            if (strcmp(command, "a") == 0) {
                if (validate_speed(value)) {
                    send_motor_command('A', 1, value);
                }
            } else if (strcmp(command, "-a") == 0) {
                if (validate_speed(value)) {
                    send_motor_command('A', -1, value);
                }
            } else if (strcmp(command, "b") == 0) {
                if (validate_speed(value)) {
                    send_motor_command('B', 1, value);
                }
            } else if (strcmp(command, "-b") == 0) {
                if (validate_speed(value)) {
                    send_motor_command('B', -1, value);
                }
            } else {
                printf(COLOR_RED "알 수 없는 명령어: %s\n" COLOR_RESET, command);
            }
        } else if (sscanf(input, "%s", command) == 1) {
            if (strcmp(command, "stop") == 0) {
                send_motor_command('S', 0, 0);
            } else if (strcmp(command, "status") == 0) {
                print_status();
            } else if (strcmp(command, "help") == 0) {
                print_help();
            } else if (strcmp(command, "clear") == 0) {
                clear_screen();
                print_header();
            } else if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0) {
                cleanup_and_exit(0);
            } else {
                printf(COLOR_RED "알 수 없는 명령어: %s\n" COLOR_RESET, command);
                printf("'help'를 입력하여 도움말을 확인하세요.\n");
            }
        } else {
            printf(COLOR_RED "잘못된 입력 형식입니다.\n" COLOR_RESET);
            printf("'help'를 입력하여 도움말을 확인하세요.\n");
        }
    }
}

// 메인 함수
int main(void) {
    // 시그널 핸들러 등록
    signal(SIGINT, cleanup_and_exit);
    signal(SIGTERM, cleanup_and_exit);
    
    // 화면 초기화
    clear_screen();
    print_header();
    
    // 디바이스 열기
    if (open_device() < 0) {
        return 1;
    }
    
    printf(COLOR_GREEN "✓ L298N 모터 드라이버에 연결되었습니다.\n" COLOR_RESET);
    printf(COLOR_YELLOW "주의: Ctrl+C로 안전하게 종료할 수 있습니다.\n" COLOR_RESET);
    
    print_help();
    print_status();
    
    // 메인 제어 루프 시작
    motor_control_loop();
    
    // 정리 및 종료
    cleanup_and_exit(0);
    return 0;
}