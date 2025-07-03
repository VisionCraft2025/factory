#!/bin/bash

BROKER="mqtt.kwon.pics"
CMD_TOPIC="robot_arm/cmd"
STATUS_TOPIC="robot_arm/status"

echo "Robot Arm MQTT 테스트"
echo "브로커: $BROKER"
echo "명령 토픽: $CMD_TOPIC"
echo "상태 토픽: $STATUS_TOPIC"
echo ""

# 브로커 연결 테스트
echo "브로커 연결 테스트..."
if timeout 5 mosquitto_pub -h $BROKER -t test/connection -m "ping" 2>/dev/null; then
    echo "✓ 브로커 연결 성공"
else
    echo "✗ 브로커 연결 실패"
    exit 1
fi

echo ""
echo "Robot Arm 명령어 테스트:"

while true; do
    echo ""
    echo "명령어를 선택하세요:"
    echo "1) auto_on / pickup - 자동 시퀀스 시작"
    echo "2) auto_off / stop - 자동 시퀀스 중지"
    echo "3) init - 초기화"
    echo "4) base_left - 베이스 좌측"
    echo "5) base_right - 베이스 우측"
    echo "6) base_center - 베이스 중앙"
    echo "7) servo0 90 - 베이스 90도"
    echo "8) servo1 45 - 어깨 45도"
    echo "9) 사용자 정의 명령 입력"
    echo "0) 종료"
    
    read -p "> " choice
    
    case "$choice" in
        "1")
            mosquitto_pub -h $BROKER -t $CMD_TOPIC -m "pickup"
            echo "명령 전송: pickup"
            ;;
        "2")
            mosquitto_pub -h $BROKER -t $CMD_TOPIC -m "stop"
            echo "명령 전송: stop"
            ;;
        "3")
            mosquitto_pub -h $BROKER -t $CMD_TOPIC -m "init"
            echo "명령 전송: init"
            ;;
        "4")
            mosquitto_pub -h $BROKER -t $CMD_TOPIC -m "base_left"
            echo "명령 전송: base_left"
            ;;
        "5")
            mosquitto_pub -h $BROKER -t $CMD_TOPIC -m "base_right"
            echo "명령 전송: base_right"
            ;;
        "6")
            mosquitto_pub -h $BROKER -t $CMD_TOPIC -m "base_center"
            echo "명령 전송: base_center"
            ;;
        "7")
            mosquitto_pub -h $BROKER -t $CMD_TOPIC -m "servo0 90"
            echo "명령 전송: servo0 90"
            ;;
        "8")
            mosquitto_pub -h $BROKER -t $CMD_TOPIC -m "servo1 45"
            echo "명령 전송: servo1 45"
            ;;
        "9")
            read -p "명령어 입력: " custom_cmd
            mosquitto_pub -h $BROKER -t $CMD_TOPIC -m "$custom_cmd"
            echo "명령 전송: $custom_cmd"
            ;;
        "0")
            echo "테스트 종료"
            break
            ;;
        *)
            echo "잘못된 선택"
            ;;
    esac
done