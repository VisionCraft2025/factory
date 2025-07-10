#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

extern "C" {
    #include "MQTTClient.h"
}

#define DEVICE_PATH "/dev/l298n_motor"
#define MAX_COMMAND_LEN 50

// 색상 정의
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

class MQTTMotorController {
private:
    MQTTClient client;
    std::string broker_url;
    std::string client_id;
    std::string ca_cert_path;
    std::string subscribe_topic;
    int device_fd;
    bool running;

    // 현재 모터 상태
    int current_motor_a_speed;
    int current_motor_b_speed;
    int current_motor_a_dir;   // -1: 역방향, 0: 정지, 1: 정방향
    int current_motor_b_dir;

public:
    MQTTMotorController(const std::string& url, const std::string& id, 
                       const std::string& cert_path, const std::string& topic) 
        : broker_url(url), client_id(id), ca_cert_path(cert_path), 
          subscribe_topic(topic), device_fd(-1), running(true),
          current_motor_a_speed(0), current_motor_b_speed(0),
          current_motor_a_dir(0), current_motor_b_dir(0) {
        
        int rc = MQTTClient_create(&client, broker_url.c_str(), client_id.c_str(),
                                   MQTTCLIENT_PERSISTENCE_NONE, nullptr);
        if (rc != MQTTCLIENT_SUCCESS) {
            std::cerr << COLOR_RED << "Failed to create MQTT client, return code: " << rc << COLOR_RESET << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    ~MQTTMotorController() {
        cleanup();
        MQTTClient_destroy(&client);
    }

    // 디바이스 열기
    bool open_device() {
        device_fd = open(DEVICE_PATH, O_WRONLY);
        if (device_fd < 0) {
            std::cerr << COLOR_RED << "오류: 디바이스 파일을 열 수 없습니다: " << DEVICE_PATH << COLOR_RESET << std::endl;
            std::cerr << "모듈이 로드되었는지 확인하세요: sudo insmod l298n_motor_driver.ko" << std::endl;
            return false;
        }
        std::cout << COLOR_GREEN << "✓ L298N 모터 드라이버에 연결되었습니다." << COLOR_RESET << std::endl;
        return true;
    }

    // 디바이스 닫기
    void close_device() {
        if (device_fd >= 0) {
            // 모든 모터 정지
            send_motor_command('S', 0, 0);
            close(device_fd);
            device_fd = -1;
        }
    }

    // 모터 명령 전송
    void send_motor_command(char motor, int direction, int speed) {
        if (device_fd < 0) {
            std::cerr << COLOR_RED << "디바이스가 열려있지 않습니다." << COLOR_RESET << std::endl;
            return;
        }

        char command[MAX_COMMAND_LEN];
        snprintf(command, sizeof(command), "%c %d %d", motor, direction, speed);
        
        if (write(device_fd, command, strlen(command)) < 0) {
            std::cerr << COLOR_RED << "오류: 명령 전송 실패 - " << strerror(errno) << COLOR_RESET << std::endl;
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
        
        std::cout << COLOR_GREEN << "✓ 모터 명령 실행: " << command << COLOR_RESET << std::endl;
    }

    // MQTT 메시지 파싱 및 모터 제어
    void process_motor_command(const std::string& message) {
        std::cout << COLOR_CYAN << "받은 명령: " << message << COLOR_RESET << std::endl;

        // JSON 형태가 아닌 간단한 텍스트 명령어 파싱
        // 예: "a 50", "-a 30", "b 70", "stop" 등
        
        std::string command;
        int value = 0;
        
        // "stop" 명령 처리
        if (message == "stop" || message == "STOP") {
            send_motor_command('S', 0, 0);
            return;
        }
        
        // 공백으로 분리된 명령어 파싱
        size_t space_pos = message.find(' ');
        if (space_pos != std::string::npos) {
            command = message.substr(0, space_pos);
            try {
                value = std::stoi(message.substr(space_pos + 1));
            } catch (const std::exception& e) {
                std::cerr << COLOR_RED << "잘못된 속도 값: " << message << COLOR_RESET << std::endl;
                return;
            }
            
            // 속도 유효성 검사
            if (value < 0 || value > 100) {
                std::cerr << COLOR_RED << "속도는 0-100 범위여야 합니다: " << value << COLOR_RESET << std::endl;
                return;
            }
            
            // 명령어 처리
            if (command == "a" || command == "A") {
                send_motor_command('A', 1, value);
            } else if (command == "-a" || command == "-A") {
                send_motor_command('A', -1, value);
            } else if (command == "b" || command == "B") {
                send_motor_command('B', 1, value);
            } else if (command == "-b" || command == "-B") {
                send_motor_command('B', -1, value);
            } else {
                std::cerr << COLOR_RED << "알 수 없는 명령어: " << command << COLOR_RESET << std::endl;
            }
        } else {
            // 단일 숫자 명령 (모터A 제어)
            try {
                int num_value = std::stoi(message);
                if (num_value == 0) {
                    send_motor_command('S', 0, 0);
                } else if (num_value > 0 && num_value <= 100) {
                    send_motor_command('A', 1, num_value);
                } else if (num_value < 0 && num_value >= -100) {
                    send_motor_command('A', -1, -num_value);
                } else {
                    std::cerr << COLOR_RED << "유효하지 않은 속도: " << num_value << COLOR_RESET << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << COLOR_RED << "알 수 없는 명령: " << message << COLOR_RESET << std::endl;
            }
        }
    }

    // MQTT 메시지 도착 콜백 (정적 함수)
    static int message_arrived(void* context, char* topicName, int topicLen, MQTTClient_message* message) {
        MQTTMotorController* controller = static_cast<MQTTMotorController*>(context);
        
        std::string payload(static_cast<char*>(message->payload), message->payloadlen);
        std::cout << COLOR_BLUE << "토픽 '" << topicName << "'에서 메시지 도착: " << payload << COLOR_RESET << std::endl;
        
        controller->process_motor_command(payload);
        
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        return 1;
    }

    // 연결 끊김 콜백
    static void connection_lost(void* context, char* cause) {
        std::cout << COLOR_YELLOW << "MQTT 연결이 끊어졌습니다: " << (cause ? cause : "알 수 없는 이유") << COLOR_RESET << std::endl;
    }

    bool connect() {
        MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
        MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;

        // 인증서 파일 존재 확인
        std::ifstream cert_file(ca_cert_path);
        if (!cert_file.good()) {
            std::cerr << COLOR_RED << "CA certificate file not found: " << ca_cert_path << COLOR_RESET << std::endl;
            return false;
        }
        cert_file.close();

        // SSL 옵션 설정
        ssl_opts.trustStore = ca_cert_path.c_str();
        ssl_opts.verify = 1;
        ssl_opts.enableServerCertAuth = 1;

        // 연결 옵션 설정
        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 1;
        conn_opts.ssl = &ssl_opts;

        // 콜백 설정
        MQTTClient_setCallbacks(client, this, connection_lost, message_arrived, nullptr);

        std::cout << COLOR_CYAN << "MQTT 브로커에 연결 중: " << broker_url << COLOR_RESET << std::endl;

        int rc = MQTTClient_connect(client, &conn_opts);
        if (rc != MQTTCLIENT_SUCCESS) {
            std::cerr << COLOR_RED << "MQTT 연결 실패, return code: " << rc << COLOR_RESET << std::endl;
            return false;
        }

        std::cout << COLOR_GREEN << "✓ MQTT 브로커에 연결되었습니다!" << COLOR_RESET << std::endl;

        // 토픽 구독
        rc = MQTTClient_subscribe(client, subscribe_topic.c_str(), 1);
        if (rc != MQTTCLIENT_SUCCESS) {
            std::cerr << COLOR_RED << "토픽 구독 실패: " << subscribe_topic << COLOR_RESET << std::endl;
            return false;
        }

        std::cout << COLOR_GREEN << "✓ 토픽 구독: " << subscribe_topic << COLOR_RESET << std::endl;
        return true;
    }

    void run() {
        std::cout << COLOR_BOLD << "\n=== MQTT 모터 제어 시스템 시작 ===" << COLOR_RESET << std::endl;
        std::cout << COLOR_CYAN << "구독 토픽: " << subscribe_topic << COLOR_RESET << std::endl;
        std::cout << COLOR_YELLOW << "Ctrl+C로 안전하게 종료할 수 있습니다." << COLOR_RESET << std::endl;
        std::cout << COLOR_CYAN << "명령어 예시:" << COLOR_RESET << std::endl;
        std::cout << "  a 50      - 모터A 정방향 50%" << std::endl;
        std::cout << "  -a 30     - 모터A 역방향 30%" << std::endl;
        std::cout << "  b 70      - 모터B 정방향 70%" << std::endl;
        std::cout << "  stop      - 모든 모터 정지" << std::endl;
        std::cout << "  50        - 모터A 정방향 50%" << std::endl;
        std::cout << "  -25       - 모터A 역방향 25%" << std::endl;

        while (running) {
            sleep(1);  // 메시지 처리를 위해 대기
        }
    }

    void stop() {
        running = false;
    }

    void cleanup() {
        if (MQTTClient_isConnected(client)) {
            MQTTClient_disconnect(client, 10000);
            std::cout << COLOR_YELLOW << "MQTT 연결이 해제되었습니다." << COLOR_RESET << std::endl;
        }
        close_device();
    }
};

// 전역 컨트롤러 포인터 (시그널 핸들러용)
MQTTMotorController* g_controller = nullptr;

// 시그널 핸들러
void signal_handler(int sig) {
    std::cout << COLOR_YELLOW << "\n\n프로그램을 종료합니다..." << COLOR_RESET << std::endl;
    if (g_controller) {
        g_controller->stop();
        g_controller->cleanup();
    }
    std::cout << COLOR_GREEN << "안전하게 종료되었습니다." << COLOR_RESET << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    // 명령행 인자 확인
    if (argc != 2) {
        std::cerr << COLOR_RED << "사용법: " << argv[0] << " <토픽명>" << COLOR_RESET << std::endl;
        std::cerr << "예시: " << argv[0] << " motor/control" << std::endl;
        return EXIT_FAILURE;
    }

    // 시그널 핸들러 등록
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        // 설정
        std::string broker_url = "ssl://mqtt.kwon.pics:8883";
        std::string client_id = "RaspberryPiMotorController";
        std::string ca_cert_path = "../ca.crt";
        std::string subscribe_topic = argv[1];  // 명령행 인자에서 토픽명 가져오기

        // 컨트롤러 생성
        MQTTMotorController controller(broker_url, client_id, ca_cert_path, subscribe_topic);
        g_controller = &controller;

        // 모터 디바이스 열기
        if (!controller.open_device()) {
            return EXIT_FAILURE;
        }

        // MQTT 브로커에 연결
        if (!controller.connect()) {
            return EXIT_FAILURE;
        }

        // 메인 루프 실행
        controller.run();

    } catch (const std::exception& e) {
        std::cerr << COLOR_RED << "예외 발생: " << e.what() << COLOR_RESET << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}