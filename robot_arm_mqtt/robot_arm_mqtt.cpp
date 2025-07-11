#include <QtMqtt/QMqttClient>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QtMqtt/QMqttSubscription>
#include <QtMqtt/QMqttTopicFilter>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QMqttClient client;
    client.setHostname("mqtt.kwon.pics");   // 브로커 주소
    client.setPort(1883);   // 일반 MQTT 포트 (TLS면 8883)
    client.connectToHost();

    QObject::connect(&client, &QMqttClient::connected, [&]() {
        qInfo("Robot Arm MQTT connected");
        client.subscribe(QMqttTopicFilter("robot_arm/cmd"));  // "robot_arm/cmd" 토픽
    });

    QObject::connect(&client, &QMqttClient::messageReceived,
        [&](const QByteArray &message, const QMqttTopicName &topic) {
            QString cmd = QString::fromUtf8(message).trimmed().toLower();
            qInfo() << "Received on" << topic.name() << ":" << cmd;

            QFile robotArm("/dev/robot_arm");
            if (!robotArm.open(QIODevice::WriteOnly | QIODevice::Text)) {
                qWarning("Failed to open /dev/robot_arm");
                return;
            }

            QTextStream out(&robotArm);
            
            // 기본 명령어들
            if (cmd == "on" || cmd == "off" || cmd == "init") {
                out << cmd << "\n";
            }
            // 별칭 명령어들
            else if (cmd == "pickup" || cmd == "start") {
                out << "on\n";
            }
            else if (cmd == "stop" || cmd == "halt") {
                out << "off\n";
            }
            // 베이스 제어 명령어들
            else if (cmd == "base_left") {
                out << "servo0 30\n";
            }
            else if (cmd == "base_right") {
                out << "servo0 150\n";
            }
            else if (cmd == "base_center") {
                out << "servo0 90\n";
            }
            // 개별 서보 명령어 (servo0 90, servo1 45 등)
            else if (cmd.startsWith("servo") && cmd.contains(" ")) {
                out << cmd << "\n";
            }
            else {
                qWarning("Invalid command received");
            }
        }
    );

    QTimer::singleShot(10000, &client, SLOT(connectToHost()));  // 10초 후 재연결

    return app.exec();
}