#!/bin/bash

DRIVER_NAME=robot_arm_driver
DEVICE_NAME=robot_arm
USER_APP=robot_arm_user
MAJOR_NUM=

echo "=== Robot Arm Setup Script ==="


echo "<기존 모듈 제거>"
sudo rmmod ${DRIVER_NAME} 2>/dev/null

echo "<빌드 시작>"
make clean > /dev/null
make || { echo "!빌드 실패"; exit 1; }

echo "<커널 모듈 삽입>"
sudo insmod ${DRIVER_NAME}.ko || { echo "!모듈 삽입 실패"; exit 1; }

echo "<major 번호 확인>"
sleep 0.5
MAJOR_NUM=$(dmesg | grep "Robot Arm driver loaded" | tail -1 | grep -oP 'Major: \K[0-9]+')

if [ -z "$MAJOR_NUM" ]; then
    echo "!major 번호 오류"
    sudo rmmod ${DRIVER_NAME}
    exit 1
fi
echo "major 번호: $MAJOR_NUM"

echo "</dev/$DEVICE_NAME 생성>"
sudo mknod /dev/$DEVICE_NAME c $MAJOR_NUM 0
sudo chmod 666 /dev/$DEVICE_NAME

echo "<GPIO 권한 설정>"
# GPIO 접근을 위한 추가 권한 설정
sudo chmod 666 /sys/class/gpio/export 2>/dev/null
sudo chmod 666 /sys/class/gpio/unexport 2>/dev/null

echo "<서보 모터 초기화>"
echo "init" > /dev/$DEVICE_NAME

echo "<시스템 준비 완료>"
echo "디바이스: /dev/$DEVICE_NAME"
echo "사용 가능한 명령:"
echo "  auto_on     - 자동 시퀀스 시작"
echo "  auto_off    - 자동 시퀀스 중지" 
echo "  init        - 서보 초기화"
echo "  servo[0-3] [angle] - 개별 서보 제어"

echo ""
echo "<유저 프로그램 실행>"
./$USER_APP

echo ""
echo "<종료 후 정리>"
echo "자동 모드 중지..."
echo "auto_off" > /dev/$DEVICE_NAME 2>/dev/null

echo "<모듈 제거 및 장치 파일 삭제를 원하면 엔터를 누르세요>"
read

echo "시스템 정리 중..."
sudo rmmod ${DRIVER_NAME} 2>/dev/null
sudo rm -f /dev/$DEVICE_NAME

echo "Robot Arm 시스템이 정리되었습니다."
