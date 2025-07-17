#ifndef CERT_UTILS_HPP
#define CERT_UTILS_HPP

#include <string>
#include <openssl/x509.h>
#include <openssl/pem.h>

/*
 * 인증서에서 device_id 추출 유틸리티
 */
class CertUtils {
public:
    /*
     * 클라이언트 인증서에서 CN(Common Name) 추출
     * - 인증서 파일 경로를 받아서 CN 필드 값 반환
     * - generate_client_cert.sh에서 CN에 device_id를 설정하므로 이를 활용
     * - 반환값: device_id 문자열 (실패 시 빈 문자열)
     */
    static std::string extractDeviceIdFromCert(const std::string& cert_path);
    
    /*
     * 인증서 파일 존재 여부 확인
     * - 클라이언트 인증서와 개인키 파일이 모두 존재하는지 확인
     * - 반환값: true(모든 파일 존재), false(파일 누락)
     */
    static bool validateCertFiles(const std::string& cert_path, const std::string& key_path);
};

#endif