    #!/bin/bash

    DRIVER_NAME=feeder_driver
    DEVICE_NAME=feeder
    USER_APP=feeder_user
    MQTT_APP=feeder_mqtt
    MAJOR_NUM=

    echo "<기존 모듈 제거>"
    sudo rmmod ${DRIVER_NAME} 2>/dev/null

    echo "<MQTT 환경변수 설정>"
    MQTT_PKGCONFIG_DIR=$(find $HOME/tls_mqtt/qt6_local -type d -name pkgconfig | grep aarch64 | head -n 1)

    if [ -z "$MQTT_PKGCONFIG_DIR" ]; then
        echo "!MQTT 패키지 경로를 찾을 수 없습니다."
        exit 1
    fi

    export PKG_CONFIG_PATH="$MQTT_PKGCONFIG_DIR:$PKG_CONFIG_PATH"

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

    # echo "<유저 프로그램 실행>"
    # ./$USER_APP

    # MQTT 프로그램은 수동 실행
    # echo "<MQTT 피더>"
    # echo "./$MQTT_APP"

    # MQTT 실행 전 LD_LIBRARY_PATH 설정
    MQTT_LIBDIR=$(find $HOME/tls_mqtt/qt6_local -type f -name "libQt6Mqtt.so*" | xargs dirname | head -n 1)
    export LD_LIBRARY_PATH="$MQTT_LIBDIR:$LD_LIBRARY_PATH"

    echo "유저 모드 또는 MQTT 모드 선택(1/2 입력):"
    select MODE in "USER" "MQTT"; do
        case "$MODE" in
            "USER")
                echo "<유저 프로그램 실행>"
                ./$USER_APP
                break
                ;;
            "MQTT")
                echo "<MQTT 피더 실행>"
                ./$MQTT_APP
                break
                ;;
            *)
                echo "잘못된 선택"
                ;;
        esac
    done
