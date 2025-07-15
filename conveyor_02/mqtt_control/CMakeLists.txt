cmake_minimum_required(VERSION 3.10)
project(mqtt_motor_control)

set(CMAKE_CXX_STANDARD 11)

# paho.mqtt.c 경로
set(PAHO_MQTT_C_DIR "/home/kwon/dev/cpp_libs/paho.mqtt.c")

# 헤더 파일 경로
include_directories(${PAHO_MQTT_C_DIR}/src)

# 라이브러리 경로
link_directories(${PAHO_MQTT_C_DIR}/build/src)

# 실행파일을 build 디렉토리에 생성
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(mqtt_motor_control mqtt_motor_control.cpp)

# SSL 지원 라이브러리 링크
target_link_libraries(mqtt_motor_control paho-mqtt3cs ssl crypto)