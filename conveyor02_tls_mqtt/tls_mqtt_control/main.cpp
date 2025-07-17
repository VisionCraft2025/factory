#include "mqtt_tls_conveyor.hpp"
#include <iostream>
#include <signal.h>
#include <memory>

// 전역 컨트롤러 포인터 (시그널 핸들러용)
std::unique_ptr<MqttTlsConveyor> g_controller = nullptr;

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
    // 시그널 핸들러 등록
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        // 기본 설정
        MqttConfig config;
        config.broker_host = "mqtt.kwon.pics";
        config.broker_port = 8883;
        config.ca_cert_path = "certs/ca.crt";
        config.client_cert_path = "certs/conveyor_02.crt";
        config.client_key_path = "certs/conveyor_02.key";
        config.subscribe_topic = "factory/{device_id}/cmd";
        
        // 명령행 인자로 토픽 변경 가능
        if (argc == 2) {
            config.subscribe_topic = argv[1];
            std::cout << COLOR_CYAN << "사용자 정의 토픽: " << config.subscribe_topic << COLOR_RESET << std::endl;
        }

        // 컨트롤러 생성
        g_controller = std::make_unique<MqttTlsConveyor>(config);

        // 모터 디바이스 열기
        if (!g_controller->open_device()) {
            return EXIT_FAILURE;
        }

        // MQTT 브로커에 연결
        if (!g_controller->connect()) {
            return EXIT_FAILURE;
        }

        // 메인 루프 실행
        g_controller->run();

    } catch (const std::exception& e) {
        std::cerr << COLOR_RED << "예외 발생: " << e.what() << COLOR_RESET << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}