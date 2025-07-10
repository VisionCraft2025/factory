#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <fstream>  // 파일 존재 확인용

extern "C" {
    #include "MQTTClient.h"
}

class MQTTSSLClient {
private:
    MQTTClient client;
    std::string broker_url;
    std::string client_id;
    std::string ca_cert_path;

public:
    MQTTSSLClient(const std::string& url, const std::string& id, const std::string& cert_path) 
        : broker_url(url), client_id(id), ca_cert_path(cert_path) {
        
        int rc = MQTTClient_create(&client, broker_url.c_str(), client_id.c_str(),
                                   MQTTCLIENT_PERSISTENCE_NONE, nullptr);
        if (rc != MQTTCLIENT_SUCCESS) {
            std::cerr << "Failed to create MQTT client, return code: " << rc << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    ~MQTTSSLClient() {
        MQTTClient_destroy(&client);
    }

    bool connect() {
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;

    // 인증서 파일 존재 확인
    std::ifstream cert_file(ca_cert_path);
    if (!cert_file.good()) {
        std::cerr << "CA certificate file not found: " << ca_cert_path << std::endl;
        return false;
    }
    cert_file.close();

    // SSL 옵션 설정
    ssl_opts.trustStore = ca_cert_path.c_str();
    ssl_opts.verify = 1;
    ssl_opts.enableServerCertAuth = 1;
    // sslVersion 주석처리로 기본값 사용

    // 연결 옵션 설정
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.ssl = &ssl_opts;

    std::cout << "Connecting to broker: " << broker_url << std::endl;
    std::cout << "Using CA certificate: " << ca_cert_path << std::endl;

    int rc = MQTTClient_connect(client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        std::cerr << "Failed to connect, return code: " << rc << std::endl;
        return false;
    }

    std::cout << "Connected to MQTT broker successfully!" << std::endl;
    return true;
}

    bool publish(const std::string& topic, const std::string& payload) {
        MQTTClient_message pubmsg = MQTTClient_message_initializer;
        MQTTClient_deliveryToken token;

        pubmsg.payload = const_cast<char*>(payload.c_str());
        pubmsg.payloadlen = static_cast<int>(payload.length());
        pubmsg.qos = 1;
        pubmsg.retained = 0;

        std::cout << "Publishing message: '" << payload << "' to topic: '" << topic << "'" << std::endl;

        int rc = MQTTClient_publishMessage(client, topic.c_str(), &pubmsg, &token);
        if (rc != MQTTCLIENT_SUCCESS) {
            std::cerr << "Failed to publish message, return code: " << rc << std::endl;
            return false;
        }

        // 메시지 전송 완료 대기
        rc = MQTTClient_waitForCompletion(client, token, 5000);
        if (rc != MQTTCLIENT_SUCCESS) {
            std::cerr << "Failed to wait for completion, return code: " << rc << std::endl;
            return false;
        }

        std::cout << "Message published successfully!" << std::endl;
        return true;
    }

    void disconnect() {
        MQTTClient_disconnect(client, 10000);
        std::cout << "Disconnected from MQTT broker" << std::endl;
    }
};

int main() {
    try {
        // SSL 연결을 위한 설정
        std::string broker_url = "ssl://mqtt.kwon.pics:8883";  // ssl:// 프로토콜 추가
        std::string client_id = "RaspberryPiClient";
        std::string ca_cert_path = "../ca.crt";

        // MQTT 클라이언트 생성
        MQTTSSLClient mqtt_client(broker_url, client_id, ca_cert_path);

        // 브로커에 연결
        if (!mqtt_client.connect()) {
            std::cerr << "Connection failed!" << std::endl;
            return EXIT_FAILURE;
        }

        // Hello 메시지 발행
        std::string topic = "test/hello";
        std::string message = "Hello from Raspberry Pi!";
        
        if (!mqtt_client.publish(topic, message)) {
            std::cerr << "Publish failed!" << std::endl;
            return EXIT_FAILURE;
        }

        // 연결 해제
        mqtt_client.disconnect();

        std::cout << "Program completed successfully!" << std::endl;
        return EXIT_SUCCESS;

    } catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}