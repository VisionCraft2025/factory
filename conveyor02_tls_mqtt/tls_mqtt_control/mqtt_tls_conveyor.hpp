#ifndef MQTT_TLS_CONVEYOR_HPP
#define MQTT_TLS_CONVEYOR_HPP

#include <mosquitto.h>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

// 색상 정의
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

#define DEVICE_PATH "/dev/l298n_motor"
#define MAX_COMMAND_LEN 50

struct MqttConfig {
    std::string broker_host = "mqtt.kwon.pics";
    int broker_port = 8883;
    std::string client_cert_path = "certs/conveyor_01.crt";
    std::string client_key_path = "certs/conveyor_01.key";
    std::string ca_cert_path = "certs/ca.crt";
    int keepalive = 60;
    bool use_tls = true;
    bool auto_detect_device_id = true;
    std::string subscribe_topic = "factory/{device_id}/cmd";
    
    MqttConfig() = default;
    MqttConfig(const std::string& host, int port, const std::string& cert, const std::string& key, 
               const std::string& ca, int alive, bool tls, bool auto_id, const std::string& topic)
        : broker_host(host), broker_port(port), client_cert_path(cert), 
          client_key_path(key), ca_cert_path(ca), keepalive(alive), 
          use_tls(tls), auto_detect_device_id(auto_id), subscribe_topic(topic) {}
};

class MqttTlsConveyor {
private:
    struct mosquitto *mosq;
    MqttConfig config;
    bool connected;
    bool running;
    int device_fd;
    std::string device_id;
    
    // 모터 상태
    int current_motor_a_speed;
    int current_motor_b_speed;
    int current_motor_a_dir;   // -1: 역방향, 0: 정지, 1: 정방향
    int current_motor_b_dir;

    // 콜백 함수
    static void on_connect_callback(struct mosquitto *mosq, void *userdata, int result);
    static void on_disconnect_callback(struct mosquitto *mosq, void *userdata, int result);
    static void on_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);
    static void on_publish_callback(struct mosquitto *mosq, void *userdata, int mid);

public:
    MqttTlsConveyor(const MqttConfig &cfg = MqttConfig());
    ~MqttTlsConveyor();

    // MQTT 연결 관리
    bool connect();
    void disconnect();
    bool isConnected() const { return connected; }
    
    // 디바이스 제어
    bool open_device();
    void close_device();
    void send_motor_command(char motor, int direction, int speed);
    void process_motor_command(const std::string& topic, const std::string& message);
    
    // 메시지 발행
    bool publishStatus(const std::string &status, const std::string &metadata = "");
    
    // 실행 제어
    void run();
    void stop();
    void cleanup();
    
    // 현재 device_id 반환
    std::string getDeviceId() const { return device_id; }

private:
    long long getCurrentTimestampMs();
    std::string createJsonMessage(const std::string &status, const std::string &metadata);
    void extractDeviceIdFromCert();
    void setupCallbacks();
    void setupTls();
    std::string getFormattedTopic(const std::string& topic_template);
};

#endif