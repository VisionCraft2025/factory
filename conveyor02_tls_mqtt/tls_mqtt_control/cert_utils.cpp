#include "cert_utils.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

std::string CertUtils::extractDeviceIdFromCert(const std::string& cert_path) {
    // PEM 형식 인증서 파일 열기
    FILE* cert_file = fopen(cert_path.c_str(), "r");
    if (!cert_file) {
        std::cerr << "[CERT] 인증서 파일 열기 실패: " << cert_path << std::endl;
        return "";
    }
    
    // PEM 형식을 X509 구조체로 파싱
    X509* cert = PEM_read_X509(cert_file, nullptr, nullptr, nullptr);
    fclose(cert_file);
    
    if (!cert) {
        std::cerr << "[CERT] 인증서 파싱 실패" << std::endl;
        return "";
    }
    
    // 인증서의 Subject Name 구조체 가져오기
    X509_NAME* subject = X509_get_subject_name(cert);
    if (!subject) {
        X509_free(cert);
        return "";
    }
    
    // Subject Name에서 CN(Common Name) 필드의 인덱스 찾기
    int cn_index = X509_NAME_get_index_by_NID(subject, NID_commonName, -1);
    if (cn_index < 0) {
        std::cerr << "[CERT] CN 필드를 찾을 수 없음" << std::endl;
        X509_free(cert);
        return "";
    }
    
    // CN 필드의 실제 데이터 엔트리 가져오기
    X509_NAME_ENTRY* cn_entry = X509_NAME_get_entry(subject, cn_index);
    if (!cn_entry) {
        X509_free(cert);
        return "";
    }
    
    // CN 엔트리에서 실제 값 데이터 추출
    ASN1_STRING* cn_data = X509_NAME_ENTRY_get_data(cn_entry);
    if (!cn_data) {
        X509_free(cert);
        return "";
    }
    
    // ASN.1 문자열을 UTF-8 형식으로 변환
    unsigned char* cn_str = nullptr;
    int len = ASN1_STRING_to_UTF8(&cn_str, cn_data);
    
    std::string device_id;
    if (len > 0 && cn_str) {
        // UTF-8 바이트 배열을 C++ string으로 변환
        device_id = std::string(reinterpret_cast<char*>(cn_str), len);
        OPENSSL_free(cn_str);
    }
    
    X509_free(cert);
    
    std::cout << "[CERT] 인증서에서 추출된 device_id: " << device_id << std::endl;
    return device_id;
}

bool CertUtils::validateCertFiles(const std::string& cert_path, const std::string& key_path) {
    // C++17 filesystem 라이브러리를 사용한 파일 존재 여부 확인
    bool cert_exists = std::filesystem::exists(cert_path);
    bool key_exists = std::filesystem::exists(key_path);
    
    // 누락된 파일에 대한 구체적인 오류 메시지 출력
    if (!cert_exists) {
        std::cerr << "[CERT] 인증서 파일 없음: " << cert_path << std::endl;
    }
    if (!key_exists) {
        std::cerr << "[CERT] 개인키 파일 없음: " << key_path << std::endl;
    }
    
    // 두 파일이 모두 존재해야 true 반환
    return cert_exists && key_exists;
}