cmake_minimum_required(VERSION 3.10)
project(tls_mqtt_conveyor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 필요한 패키지 찾기
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MOSQUITTO REQUIRED libmosquitto)

# 소스 파일 설정
set(SOURCES
    main.cpp
    mqtt_tls_conveyor.cpp
    cert_utils.cpp
)

# 헤더 파일 설정
set(HEADERS
    mqtt_tls_conveyor.hpp
    cert_utils.hpp
)

# 실행 파일 생성
add_executable(tls_mqtt_conveyor ${SOURCES} ${HEADERS})

# 인클루드 디렉토리 설정
target_include_directories(tls_mqtt_conveyor PRIVATE ${MOSQUITTO_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})

# 라이브러리 링크
target_link_libraries(tls_mqtt_conveyor ${MOSQUITTO_LIBRARIES} ${OPENSSL_LIBRARIES} pthread)

# 컴파일 옵션 설정
target_compile_options(tls_mqtt_conveyor PRIVATE -Wall -Wextra)

# 인증서 파일 복사
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/certs/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/certs/)

# 설치 설정
install(TARGETS tls_mqtt_conveyor DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/certs/ca.crt
              ${CMAKE_CURRENT_SOURCE_DIR}/certs/conveyor_02.crt
              ${CMAKE_CURRENT_SOURCE_DIR}/certs/conveyor_02.key
        DESTINATION share/${PROJECT_NAME}/certs)