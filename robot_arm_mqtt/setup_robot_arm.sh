#!/bin/bash

DRIVER_NAME=robot_arm_driver
DEVICE_NAME=robot_arm
USER_APP=robot_arm_user
MQTT_APP=robot_arm_mqtt
MAJOR_NUM=

echo "=== Robot Arm MQTT Setup ==="

echo "<기존 모듈 제거>"
sudo rmmod ${DRIVER_NAME} 2>/dev/null

echo "<MQTT 환경변수 설정>"
# Robot Arm MQTT 경로 설정
MQTT_BASE_DIR="$HOME/cv_practice/project/pproject/lab/robot_arm_pracice/robot_arm_mqtt/tls_mqtt"
MQTT_PKGCONFIG_DIR="$MQTT_BASE_DIR/qt6_local/usr/lib/aarch64-linux-gnu/pkgconfig"

if [ ! -d "$MQTT_PKGCONFIG_DIR" ]; then
    echo "!MQTT 패키지 경로를 찾을 수 없습니다."
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
MQTT_LIBDIR="$MQTT_BASE_DIR/qt6_local/usr/lib/aarch64-linux-gnu"
export LD_LIBRARY_PATH="$MQTT_LIBDIR:$LD_LIBRARY_PATH"

echo "<서보 모터 초기화>"
echo "init" > /dev/$DEVICE_NAME

echo "유저 모드 또는 MQTT 모드 선택(1/2 입력):"
select MODE in "USER" "MQTT"; do
    case "$MODE" in
        "USER")
            echo "<유저 프로그램 실행>"
            ./$USER_APP
            break
            ;;
        "MQTT")
            echo "<Robot Arm MQTT 실행>"
            ./$MQTT_APP
            break
            ;;
        *)
            echo "잘못된 선택"
            ;;
    esac
done