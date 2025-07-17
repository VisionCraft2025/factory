#!/bin/bash

DRIVER_NAME=robot_arm_driver
DEVICE_NAME=robot_arm
USER_APP=robot_arm_user
MQTT_APP=robot_arm_mqtt
MQTT_TLS_APP=robot_arm_mqtt_tls
MAJOR_NUM=

echo "=== Robot Arm MQTT Setup ==="

echo "<기존 모듈 제거>"
sudo rmmod ${DRIVER_NAME} 2>/dev/null

echo "<MQTT 환경변수 설정>"
# Qt6 MQTT 로컬 빌드 경로 설정
QT_MQTT_DIR="$HOME/dev/cpp_libs/qtmqtt/install/usr"
MQTT_PKGCONFIG_DIR="$QT_MQTT_DIR/lib/aarch64-linux-gnu/pkgconfig"

if [ ! -d "$MQTT_PKGCONFIG_DIR" ]; then
    echo "!MQTT 패키지 경로를 찾을 수 없습니다: $MQTT_PKGCONFIG_DIR"
    exit 1
fi

export PKG_CONFIG_PATH="$MQTT_PKGCONFIG_DIR:$PKG_CONFIG_PATH"

echo "<빌드 시작>"
make clean > /dev/null
make || { echo "!빌드 실패"; exit 1; }

echo "<커널 모듈 삽입>"
sudo insmod ${DRIVER_NAME}.ko || { echo "!모듈 삽입 실패"; exit 1; }

echo "<major 번호 확인>"
sleep 0.5
MAJOR_NUM=$(cat /proc/devices | grep robot_arm | awk '{print $1}')

if [ -z "$MAJOR_NUM" ]; then
    echo "!major 번호 오류"
    sudo rmmod ${DRIVER_NAME}
    exit 1
fi
echo "major 번호: $MAJOR_NUM"

echo "</dev/$DEVICE_NAME 생성>"
sudo mknod /dev/$DEVICE_NAME c $MAJOR_NUM 0
sudo chmod 666 /dev/$DEVICE_NAME

# MQTT 실행 전 LD_LIBRARY_PATH 설정
MQTT_LIBDIR="$QT_MQTT_DIR/lib/aarch64-linux-gnu"
export LD_LIBRARY_PATH="$MQTT_LIBDIR:$LD_LIBRARY_PATH"

echo "<서보 모터 초기화>"
echo "init" > /dev/$DEVICE_NAME

echo "인증서 파일 확인 중..."
ls -la /certs/ca.crt /certs/robot_arm_01.crt /certs/robot_arm_01.key 2>&1 || true

# 상대 경로로 다시 시도
if [ ! -f "/certs/ca.crt" ] && [ -f "$HOME/certs/ca.crt" ]; then
    echo "상대 경로에서 인증서 발견: $HOME/certs/"
    export CERT_PATH="$HOME/certs"
elif [ ! -f "/certs/ca.crt" ] && [ -f "./certs/ca.crt" ]; then
    echo "현재 디렉토리에서 인증서 발견: ./certs/"
    export CERT_PATH="./certs"
else
    export CERT_PATH="/certs"
fi

echo "사용할 인증서 경로: $CERT_PATH"

echo "유저 모드 또는 MQTT 모드 선택(1/2/3 입력):"
select MODE in "USER" "MQTT" "MQTT-TLS"; do
    case "$MODE" in
        "USER")
            echo "<유저 프로그램 실행>"
            ./$USER_APP
            break
            ;;
        "MQTT")
            echo "<Robot Arm MQTT 실행 (비암호화)>"
            ./$MQTT_APP
            break
            ;;
        "MQTT-TLS")
            echo "<Robot Arm MQTT 실행 (TLS 암호화)>"
            echo "인증서 확인: $CERT_PATH/ca.crt, $CERT_PATH/robot_arm_01.crt, $CERT_PATH/robot_arm_01.key"
            if [ ! -f "$CERT_PATH/ca.crt" ] || [ ! -f "$CERT_PATH/robot_arm_01.crt" ] || [ ! -f "$CERT_PATH/robot_arm_01.key" ]; then
                echo "!인증서 파일이 없습니다. TLS 연결을 위해 인증서가 필요합니다."
                echo "인증서 경로: $CERT_PATH/ca.crt, $CERT_PATH/robot_arm_01.crt, $CERT_PATH/robot_arm_01.key"
                exit 1
            fi
            CERT_PATH="$CERT_PATH" ./$MQTT_TLS_APP
            break
            ;;
        *)
            echo "잘못된 선택"
            ;;
    esac
done