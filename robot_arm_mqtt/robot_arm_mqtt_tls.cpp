#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <mosquitto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/err.h>

// 전역 변수
bool running = true;
struct mosquitto *mosq = NULL;
std::string device_id = "robot_arm"; // 기본값

// 시그널 핸들러
void handle_signal(int s) {
    running = false;
}

// 연결 콜백
void on_connect(struct mosquitto *mosq, void *obj, int reason_code) {
    if (reason_code == 0) {
        std::cout << "MQTT 브로커에 TLS로 연결 성공!" << std::endl;
        
        // 토픽 구독
        std::string topic = device_id + "/cmd";
        int rc = mosquitto_subscribe(mosq, NULL, topic.c_str(), 1);
        if (rc == MOSQ_ERR_SUCCESS) {
            std::cout << "토픽 구독 성공: " << topic << std::endl;
        } else {
            std::cerr << "토픽 구독 실패: " << mosquitto_strerror(rc) << std::endl;
        }
    } else {
        std::cerr << "연결 실패: " << mosquitto_connack_string(reason_code) << std::endl;
    }
}

// 메시지 콜백
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
    std::string topic = message->topic;
    std::string payload(static_cast<char*>(message->payload), message->payloadlen);
    
    std::cout << "수신 메시지: " << topic << " : " << payload << std::endl;
    
    // 명령어 처리
    std::ofstream robot_arm("/dev/robot_arm");
    if (!robot_arm.is_open()) {
        std::cerr << "디바이스 파일 열기 실패: /dev/robot_arm" << std::endl;
        return;
    }
    
    // 명령어 소문자로 변환
    std::string cmd = payload;
    for (auto& c : cmd) c = std::tolower(c);
    
    // 기본 명령어들
    if (cmd == "on" || cmd == "off" || cmd == "init" || cmd == "come" || cmd == "go") {
        robot_arm << cmd << std::endl;
    }
    // 별칭 명령어들
    else if (cmd == "pickup" || cmd == "start") {
        robot_arm << "on" << std::endl;
    }
    else if (cmd == "stop" || cmd == "halt") {
        robot_arm << "off" << std::endl;
    }
    // 베이스 제어 명령어들
    else if (cmd == "base_left") {
        robot_arm << "servo0 30" << std::endl;
    }
    else if (cmd == "base_right") {
        robot_arm << "servo0 150" << std::endl;
    }
    else if (cmd == "base_center") {
        robot_arm << "servo0 90" << std::endl;
    }
    // 개별 서보 명령어 (servo0 90, servo1 45 등)
    else if (cmd.find("servo") == 0 && cmd.find(" ") != std::string::npos) {
        robot_arm << cmd << std::endl;
    }
    else {
        std::cerr << "✗ 잘못된 명령어: " << cmd << std::endl;
    }
    
    robot_arm.close();
    std::cout << "✓ 명령 전송 완료: " << cmd << std::endl;
}

// 연결 해제 콜백
void on_disconnect(struct mosquitto *mosq, void *obj, int reason_code) {
    std::cout << "MQTT 연결 해제: " << mosquitto_strerror(reason_code) << std::endl;
}

// 인증서에서 device_id 추출
std::string extract_device_id_from_cert(const std::string& cert_path) {
    FILE* cert_file = fopen(cert_path.c_str(), "r");
    if (!cert_file) {
        std::cerr << "인증서 파일 열기 실패: " << cert_path << std::endl;
        return "";
    }
    
    X509* cert = PEM_read_X509(cert_file, nullptr, nullptr, nullptr);
    fclose(cert_file);
    
    if (!cert) {
        std::cerr << "인증서 파싱 실패" << std::endl;
        return "";
    }
    
    X509_NAME* subject = X509_get_subject_name(cert);
    if (!subject) {
        X509_free(cert);
        return "";
    }
    
    int cn_index = X509_NAME_get_index_by_NID(subject, NID_commonName, -1);
    if (cn_index < 0) {
        std::cerr << "CN 필드를 찾을 수 없음" << std::endl;
        X509_free(cert);
        return "";
    }
    
    X509_NAME_ENTRY* cn_entry = X509_NAME_get_entry(subject, cn_index);
    if (!cn_entry) {
        X509_free(cert);
        return "";
    }
    
    ASN1_STRING* cn_data = X509_NAME_ENTRY_get_data(cn_entry);
    if (!cn_data) {
        X509_free(cert);
        return "";
    }
    
    unsigned char* cn_str = nullptr;
    int len = ASN1_STRING_to_UTF8(&cn_str, cn_data);
    
    std::string result;
    if (len > 0 && cn_str) {
        result = std::string(reinterpret_cast<char*>(cn_str), len);
        OPENSSL_free(cn_str);
    }
    
    X509_free(cert);
    
    std::cout << "인증서에서 추출된 device_id: " << result << std::endl;
    return result;
}

int main(int argc, char *argv[]) {
    // 시그널 핸들러 설정
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    // Mosquitto 라이브러리 초기화
    mosquitto_lib_init();
    
    // 인증서 경로 가져오기
    std::string cert_dir = "/certs";
    char* env_cert_path = getenv("CERT_PATH");
    if (env_cert_path != NULL) {
        cert_dir = env_cert_path;
    }
    
    std::string cert_path = cert_dir + "/robot_arm_01.crt";
    std::string key_path = cert_dir + "/robot_arm_01.key";
    std::string ca_path = cert_dir + "/ca.crt";
    
    std::cout << "사용할 인증서 경로:" << std::endl;
    std::cout << "  CA: " << ca_path << std::endl;
    std::cout << "  인증서: " << cert_path << std::endl;
    std::cout << "  키: " << key_path << std::endl;
    
    // 파일 존재 확인
    std::ifstream ca_file(ca_path);
    std::ifstream cert_file_check(cert_path);
    std::ifstream key_file_check(key_path);
    
    if (!ca_file.good() || !cert_file_check.good() || !key_file_check.good()) {
        std::cerr << "인증서 파일을 열 수 없습니다. 경로를 확인하세요." << std::endl;
        return 1;
    }
    
    ca_file.close();
    cert_file_check.close();
    key_file_check.close();
    
    // 인증서에서 device_id 추출
    std::string extracted_id = extract_device_id_from_cert(cert_path);
    if (!extracted_id.empty()) {
        device_id = extracted_id;
    }
    
    // MQTT 클라이언트 생성
    mosq = mosquitto_new(device_id.c_str(), true, NULL);
    if (!mosq) {
        std::cerr << "MQTT 클라이언트 생성 실패" << std::endl;
        return 1;
    }
    
    // 콜백 설정
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);
    
    // TLS 설정
    std::cout << "TLS 설정 중..." << std::endl;
    int rc = mosquitto_tls_set(mosq, 
                              ca_path.c_str(),    // CA 인증서
                              NULL,               // CA 경로
                              cert_path.c_str(),  // 클라이언트 인증서
                              key_path.c_str(),   // 클라이언트 키
                              NULL);              // 비밀번호 콜백
    
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "TLS 설정 실패: " << mosquitto_strerror(rc) << std::endl;
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }
    
    // TLS 버전 설정
    rc = mosquitto_tls_opts_set(mosq, 1, "tlsv1.2", NULL);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "TLS 옵션 설정 실패: " << mosquitto_strerror(rc) << std::endl;
    }
    
    // 브로커 연결
    std::cout << "MQTT 브로커에 연결 중: mqtt.kwon.pics:8883" << std::endl;
    rc = mosquitto_connect(mosq, "mqtt.kwon.pics", 8883, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "연결 실패: " << mosquitto_strerror(rc) << std::endl;
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }
    
    // 메시지 루프 시작
    rc = mosquitto_loop_start(mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "메시지 루프 시작 실패: " << mosquitto_strerror(rc) << std::endl;
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }
    
    std::cout << "MQTT TLS 클라이언트 실행 중... (Ctrl+C로 종료)" << std::endl;
    std::cout << "Device ID: " << device_id << std::endl;
    std::cout << "구독 토픽: " << device_id << "/cmd" << std::endl;
    
    // 메인 루프
    while (running) {
        sleep(1);
    }
    
    // 정리
    std::cout << "프로그램 종료 중..." << std::endl;
    mosquitto_loop_stop(mosq, true);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    
    return 0;
}