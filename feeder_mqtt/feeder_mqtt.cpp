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
    client.setHostname("mqtt.kwon.pics");   // 브로커 주소 (확인 해야 됨)
    client.setPort(1883);   // 일반 MQTT 포트 (TLS면 8883)
    client.connectToHost();

    QObject::connect(&client, &QMqttClient::connected, [&]() {
        qInfo("MQTT connected");
        client.subscribe(QMqttTopicFilter("feeder_02/cmd"));  // "feeder/cmd" 토픽
    });

    QObject::connect(&client, &QMqttClient::messageReceived,
        [&](const QByteArray &message, const QMqttTopicName &topic) {
            QString cmd = QString::fromUtf8(message).trimmed().toLower();
            qInfo() << "Received on" << topic.name() << ":" << cmd;

            QFile feeder("/dev/feeder_02");

            if (!feeder.open(QIODevice::WriteOnly | QIODevice::Text)) {
                qWarning("Failed to open /dev/feeder_02");
                return;
            }

            QTextStream out(&feeder);
            if (cmd == "on" || cmd == "off" || cmd == "reverse" || cmd == "error" || cmd == "normal") {
                out << cmd << "\n";
            } else {
                qWarning("Invalid command received");
            }
        }
    );

    QTimer::singleShot(10000, &client, SLOT(connectToHost()));  // 10초 후 재연결함

    return app.exec();
}
