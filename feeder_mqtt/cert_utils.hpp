#ifndef CERT_UTILS_HPP
#define CERT_UTILS_HPP

#include <QString>
#include <QFile>
#include <QSslCertificate>
#include <QDebug>

class CertUtils {
public:
    // 인증서에서 Common Name(CN) 필드를 추출하여 device_id로 사용
    static QString extractDeviceIdFromCert(const QString& certPath) {
        QFile certFile(certPath);
        if (!certFile.open(QIODevice::ReadOnly)) {
            qWarning() << "인증서 파일을 열 수 없습니다:" << certPath;
            return QString();
        }
        
        QSslCertificate cert(&certFile);
        certFile.close();
        
        if (cert.isNull()) {
            qWarning() << "유효하지 않은 인증서입니다:" << certPath;
            return QString();
        }
        
        // Qt 6에서는 subjectInfo를 사용하여 CN 필드 추출
        QStringList commonNames = cert.subjectInfo(QSslCertificate::CommonName);
        if (commonNames.isEmpty()) {
            qWarning() << "인증서에서 CN 필드를 찾을 수 없습니다";
            return QString();
        }
        
        QString deviceId = commonNames.first();
        qInfo() << "인증서에서 추출한 device_id:" << deviceId;
        return deviceId;
    }
};

#endif // CERT_UTILS_HPP