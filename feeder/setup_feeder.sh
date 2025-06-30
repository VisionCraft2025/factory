#!/bin/bash

DRIVER_NAME=feeder_driver
DEVICE_NAME=feeder
USER_APP=feeder_user
MAJOR_NUM=

echo "<기존 모듈 제거>"
sudo rmmod ${DRIVER_NAME} 2>/dev/null

echo "<빌드 시작>"
make clean > /dev/null
make || { echo "!빌드 실패"; exit 1; }

echo "<커널 모듈 삽입>"
sudo insmod ${DRIVER_NAME}.ko || { echo "!모듈 삽입 실패"; exit 1; }

echo "major 번호 확인"
sleep 0.5
MAJOR_NUM=$(dmesg | grep "Feeder driver loaded" | tail -1 | grep -oP 'Major: \K[0-9]+')


if [ -z "$MAJOR_NUM" ]; then
    echo "!major 번호 오류"
    sudo rmmod ${DRIVER_NAME}
    exit 1
fi
echo "major 번호: $MAJOR_NUM"

echo "</dev/$DEVICE_NAME 생성>"
sudo mknod /dev/$DEVICE_NAME c $MAJOR_NUM 0
sudo chmod 666 /dev/$DEVICE_NAME

echo "<유저 프로그램 실행>"
./$USER_APP

#echo "<종료 후 모듈 제거 및 장치 파일 삭제>"
#sudo rmmod ${DRIVER_NAME}
#sudo rm -f /dev/$DEVICE_NAME

#echo "<엔터를 누르면 모듈을 제거합니다>"
#read

#sudo rmmod ${DRIVER_NAME}
#sudo rm -f /dev/$DEVICE_NAME
