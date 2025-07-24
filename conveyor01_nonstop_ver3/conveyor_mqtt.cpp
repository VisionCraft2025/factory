#include <QCoreApplication>
#include <qmqttclient.h>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QTimer>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    QMqttClient mqtt_client;

    mqtt_client.setHostname("mqtt.kwon.pics");
    mqtt_client.setPort(1883); 
    
    qDebug() << "MQTT 브로커 연결 시도 중... (mqtt.kwon.pics:1883)";
    mqtt_client.connectToHost();

    // 연결 성공
    QObject::connect(&mqtt_client, &QMqttClient::connected, [&](){
        qDebug() << "MQTT 브로커 연결 성공!";
        qDebug() << "클라이언트 ID:" << mqtt_client.clientId();
        
        // 토픽 구독
        auto subscription = mqtt_client.subscribe(QMqttTopicFilter("conveyor_03/cmd"));
        if (subscription) {
            qDebug() << "conveyor_03/cmd 토픽 구독 요청 전송";
            
            // 구독 성공 확인
            QObject::connect(subscription, &QMqttSubscription::stateChanged, [](QMqttSubscription::SubscriptionState state) {
                if (state == QMqttSubscription::Subscribed) {
                    qDebug() << "conveyor_03/cmd 토픽 구독 완료!";
                } else if (state == QMqttSubscription::Error) {
                    qDebug() << "토픽 구독 실패!";
                }
            });
        } else {
            qDebug() << "토픽 구독 요청 실패!";
        }
        
        // 테스트 메시지 발행 (자가 테스트)
        QTimer::singleShot(2000, [&]() {
            qDebug() << ">> 테스트 메시지 발행 중...";
            mqtt_client.publish(QMqttTopicName("conveyor_03/cmd"), "test_message");
        });
    });

    // 메시지 수신
    QObject::connect(&mqtt_client, &QMqttClient::messageReceived, [&](
        const QByteArray &message, const QMqttTopicName &topic) {
    
        QString cmd = QString::fromUtf8(message).trimmed();

        // 디바이스 파일 처리
        QFile conveyor("/dev/conveyor_mqtt");
        if (!conveyor.exists()) {
            qWarning() << "디바이스 파일이 존재하지 않음: /dev/conveyor_mqtt";
            return;
        }
        
        if (!conveyor.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "디바이스 파일 열기 실패!";
            qWarning() << "에러:" << conveyor.errorString();
            return;
        }
        
        qDebug() << "디바이스 파일 열기 성공";
    
        QTextStream out(&conveyor);
        
        if(cmd == "on"){
            out << "on\n";
            qDebug() << "컨베이어 시작 명령 전송";
        }
        else if(cmd == "off"){
            out << "off\n";
            qDebug() << "컨베이어 정지 명령 전송";
        }
        else if(cmd == "error_mode"){
            out << "error_mode\n";
            qDebug() << "에러 모드 명령 전송";
        }
        else if(cmd == "test_message"){
            qDebug() << "자가 테스트 메시지 확인됨 - 정상 동작!";
        }
        else{
            out << cmd << "\n";
            qDebug() << "기타 명령 전송:" << cmd;
        }
        
        out.flush();
        conveyor.close();
        qDebug() << "명령 전송 완료";
        
        // 상태 확인 (test_message가 아닐 때만)
        if(cmd != "test_message") {
            QFile status("/dev/conveyor_mqtt");
            if (status.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&status);
                QString statusText = in.readAll();
                status.close();
            }
        }
    });

    // 연결 끊어짐
    QObject::connect(&mqtt_client, &QMqttClient::disconnected, [&](){
        qDebug() << "MQTT 브로커 연결 끊어짐";
    });

    // 에러 발생
    QObject::connect(&mqtt_client, &QMqttClient::errorChanged, [&](QMqttClient::ClientError error){
        qDebug() << "MQTT 에러 발생:" << error;
    });

    // 상태 변경
    QObject::connect(&mqtt_client, &QMqttClient::stateChanged, [&](QMqttClient::ClientState state){
        QString stateStr;
        switch(state) {
            case QMqttClient::Disconnected: stateStr = "Disconnected"; break;
            case QMqttClient::Connecting: stateStr = "Connecting"; break;
            case QMqttClient::Connected: stateStr = "Connected"; break;
        }
        qDebug() << "MQTT 상태 변경:" << stateStr;
    });
    
    qDebug() << "MQTT 클라이언트 시작 - conveyor_03/cmd 토픽 대기 중...";
    qDebug() << "Ctrl+C로 종료하세요.";
    
    return app.exec();
}