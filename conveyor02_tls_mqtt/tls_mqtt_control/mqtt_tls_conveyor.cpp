#include "mqtt_tls_conveyor.hpp"
#include "cert_utils.hpp"
#include <sstream>
#include <algorithm>
#include <errno.h>

// 콜백 함수 구현
void MqttTlsConveyor::on_connect_callback(struct mosquitto *mosq, void *userdata, int result) {
    MqttTlsConveyor *client = static_cast<MqttTlsConveyor *>(userdata);
    if (result == 0) {
        std::cout << COLOR_GREEN << "[MQTT] TLS 연결 성공" << COLOR_RESET << std::endl;
        client->connected = true;
        
        // 토픽 구독
        std::string topic = client->getFormattedTopic(client->config.subscribe_topic);
        int rc = mosquitto_subscribe(mosq, nullptr, topic.c_str(), 1);
        if (rc == MOSQ_ERR_SUCCESS) {
            std::cout << COLOR_GREEN << "✓ 토픽 구독: " << topic << COLOR_RESET << std::endl;
        } else {
            std::cerr << COLOR_RED << "토픽 구독 실패: " << topic << COLOR_RESET << std::endl;
        }
    } else {
        std::cout << COLOR_RED << "[MQTT] TLS 연결 실패: " << result << COLOR_RESET << std::endl;
        client->connected = false;
    }
}

void MqttTlsConveyor::on_disconnect_callback(struct mosquitto *mosq, void *userdata, int result) {
    MqttTlsConveyor *client = static_cast<MqttTlsConveyor *>(userdata);
    std::cout << COLOR_YELLOW << "[MQTT] 연결 해제" << COLOR_RESET << std::endl;
    client->connected = false;
}

void MqttTlsConveyor::on_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message) {
    MqttTlsConveyor *client = static_cast<MqttTlsConveyor *>(userdata);
    
    std::string topic(message->topic);
    std::string payload(static_cast<char*>(message->payload), message->payloadlen);
    
    std::cout << COLOR_BLUE << "토픽 '" << topic << "'에서 메시지 도착: " << payload << COLOR_RESET << std::endl;
    
    client->process_motor_command(topic, payload);
}

void MqttTlsConveyor::on_publish_callback(struct mosquitto *mosq, void *userdata, int mid) {
    std::cout << COLOR_CYAN << "[MQTT] 메시지 발송 완료 (ID: " << mid << ")" << COLOR_RESET << std::endl;
}

// 생성자
MqttTlsConveyor::MqttTlsConveyor(const MqttConfig &cfg) 
    : config(cfg), connected(false), running(true), device_fd(-1),
      current_motor_a_speed(0), current_motor_b_speed(0),
      current_motor_a_dir(0), current_motor_b_dir(0) {
    
    mosquitto_lib_init();
    mosq = mosquitto_new("temp_client", true, this);
    if (!mosq) throw std::runtime_error("MQTT 클라이언트 생성 실패");
    
    setupCallbacks();

    if (config.use_tls) {
        if (!CertUtils::validateCertFiles(config.client_cert_path, config.client_key_path)) {
            throw std::runtime_error("인증서 파일을 찾을 수 없음");
        }
        
        if (config.auto_detect_device_id) {
            extractDeviceIdFromCert();
            if (!device_id.empty()) {
                mosquitto_destroy(mosq);
                mosq = mosquitto_new(device_id.c_str(), true, this);
                if (!mosq) throw std::runtime_error("MQTT 클라이언트 재생성 실패");
                setupCallbacks();
            }
        }
        
        setupTls();
    }
}

// 소멸자
MqttTlsConveyor::~MqttTlsConveyor() {
    cleanup();
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
}

// 콜백 설정
void MqttTlsConveyor::setupCallbacks() {
    mosquitto_connect_callback_set(mosq, on_connect_callback);
    mosquitto_disconnect_callback_set(mosq, on_disconnect_callback);
    mosquitto_message_callback_set(mosq, on_message_callback);
    mosquitto_publish_callback_set(mosq, on_publish_callback);
}

// TLS 설정
void MqttTlsConveyor::setupTls() {
    std::cout << COLOR_CYAN << "[MQTT] TLS 설정: " << config.ca_cert_path << COLOR_RESET << std::endl;
    
    int result = mosquitto_tls_set(mosq, config.ca_cert_path.c_str(), nullptr, 
                                  config.client_cert_path.c_str(), config.client_key_path.c_str(), nullptr);
    if (result != MOSQ_ERR_SUCCESS) {
        throw std::runtime_error("TLS 설정 실패: " + std::string(mosquitto_strerror(result)));
    }
    
    mosquitto_tls_opts_set(mosq, 1, "tlsv1.2", nullptr);
    mosquitto_tls_insecure_set(mosq, false);
}

// 인증서에서 디바이스 ID 추출
void MqttTlsConveyor::extractDeviceIdFromCert() {
    device_id = CertUtils::extractDeviceIdFromCert(config.client_cert_path);
    if (!device_id.empty()) {
        std::cout << COLOR_CYAN << "[MQTT] 인증서에서 device_id 추출: " << device_id << COLOR_RESET << std::endl;
    }
}

// MQTT 브로커 연결
bool MqttTlsConveyor::connect() {
    std::cout << COLOR_CYAN << "[MQTT] 연결 시도: " << config.broker_host << ":" << config.broker_port << COLOR_RESET << std::endl;

    int result = mosquitto_connect(mosq, config.broker_host.c_str(), config.broker_port, config.keepalive);
    if (result != MOSQ_ERR_SUCCESS) {
        std::cerr << COLOR_RED << "[MQTT] 연결 실패: " << mosquitto_strerror(result) << COLOR_RESET << std::endl;
        return false;
    }

    mosquitto_loop_start(mosq);

    // 연결 대기
    for (int i = 0; i < 50 && !connected; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return connected;
}

// MQTT 브로커 연결 해제
void MqttTlsConveyor::disconnect() {
    if (connected) {
        mosquitto_loop_stop(mosq, true);
        mosquitto_disconnect(mosq);
        connected = false;
    }
}

// 디바이스 열기
bool MqttTlsConveyor::open_device() {
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
void MqttTlsConveyor::close_device() {
    if (device_fd >= 0) {
        // 모든 모터 정지
        send_motor_command('S', 0, 0);
        close(device_fd);
        device_fd = -1;
    }
}

// 모터 명령 전송
void MqttTlsConveyor::send_motor_command(char motor, int direction, int speed) {
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
    
    // 상태 메시지 발행
    std::stringstream metadata;
    metadata << "{\"motor_a\":{\"direction\":" << current_motor_a_dir 
             << ",\"speed\":" << current_motor_a_speed 
             << "},\"motor_b\":{\"direction\":" << current_motor_b_dir 
             << ",\"speed\":" << current_motor_b_speed << "}}";
    
    publishStatus(motor == 'S' ? "stopped" : "running", metadata.str());
}

// MQTT 메시지 파싱 및 모터 제어
void MqttTlsConveyor::process_motor_command(const std::string& topic, const std::string& message) {
    std::cout << COLOR_CYAN << "토픽: " << topic << ", 메시지: " << message << COLOR_RESET << std::endl;

    // 페이로드 처리 (대소문자 구분 없음)
    std::string payload = message;
    // 소문자로 변환
    std::transform(payload.begin(), payload.end(), payload.begin(), ::tolower);
    
    if (payload == "on") {
        // 모터A 정방향 99%로 설정
        send_motor_command('A', 1, 99);
        std::cout << COLOR_GREEN << "모터 켜짐 - 모터A 정방향 속도: 99%" << COLOR_RESET << std::endl;
    } else if (payload == "off") {
        // 모터 정지
        send_motor_command('S', 0, 0);
        std::cout << COLOR_GREEN << "모터 꺼짐 - 모터 정지" << COLOR_RESET << std::endl;
    } else {
        std::cerr << COLOR_RED << "알 수 없는 명령: " << message << " (on 또는 off만 지원)" << COLOR_RESET << std::endl;
    }
}

// 현재 시간을 밀리초 단위 Unix 타임스탬프로 반환
long long MqttTlsConveyor::getCurrentTimestampMs() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return ms.count();
}

// JSON 메시지 생성
std::string MqttTlsConveyor::createJsonMessage(const std::string &status, const std::string &metadata) {
    std::stringstream json;
    json << "{"
         << "\"status\":\"" << status << "\","
         << "\"timestamp\":" << getCurrentTimestampMs();
    
    if (!metadata.empty()) {
        json << ",\"metadata\":" << metadata;
    }
    
    json << "}";
    return json.str();
}

// 상태 메시지 발행
bool MqttTlsConveyor::publishStatus(const std::string &status, const std::string &metadata) {
    if (!connected || device_id.empty()) {
        return false;
    }

    std::string topic = "factory/" + device_id + "/status";
    std::string payload = createJsonMessage(status, metadata);

    int result = mosquitto_publish(mosq, nullptr, topic.c_str(), payload.length(), payload.c_str(), 1, false);

    if (result == MOSQ_ERR_SUCCESS) {
        std::cout << COLOR_CYAN << "[MQTT] 상태 발송: " << topic << " -> " << status << COLOR_RESET << std::endl;
        return true;
    } else {
        std::cerr << COLOR_RED << "[MQTT] 발송 실패: " << mosquitto_strerror(result) << COLOR_RESET << std::endl;
        return false;
    }
}

// 토픽 템플릿에서 실제 토픽 생성
std::string MqttTlsConveyor::getFormattedTopic(const std::string& topic_template) {
    std::string result = topic_template;
    std::string placeholder = "{device_id}";
    
    size_t pos = result.find(placeholder);
    if (pos != std::string::npos && !device_id.empty()) {
        result.replace(pos, placeholder.length(), device_id);
    }
    
    return result;
}

// 메인 실행 루프
void MqttTlsConveyor::run() {
    std::cout << COLOR_BOLD << "\n=== MQTT TLS 모터 제어 시스템 시작 ===" << COLOR_RESET << std::endl;
    std::cout << COLOR_CYAN << "구독 토픽: " << getFormattedTopic(config.subscribe_topic) << COLOR_RESET << std::endl;
    std::cout << COLOR_YELLOW << "Ctrl+C로 안전하게 종료할 수 있습니다." << COLOR_RESET << std::endl;
    std::cout << COLOR_CYAN << "페이로드 명령어:" << COLOR_RESET << std::endl;
    std::cout << "  on       - 모터A 정방향 99% (켜짐)" << std::endl;
    std::cout << "  off      - 모터 정지 (꺼짐)" << std::endl;

    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));  // 메시지 처리를 위해 대기
    }
}

// 실행 중지
void MqttTlsConveyor::stop() {
    running = false;
}

// 정리
void MqttTlsConveyor::cleanup() {
    if (connected) {
        disconnect();
        std::cout << COLOR_YELLOW << "MQTT 연결이 해제되었습니다." << COLOR_RESET << std::endl;
    }
    close_device();
}