#!/bin/bash
DRIVER_NAME=conveyor_driver
DEVICE_NAME=conveyor_mqtt
USER_APP=conveyor_user
MQTT_APP=conveyor_mqtt
MAJOR_NUM=

echo "=== Conveyor System Setup ==="

# 함수: 시스템 정리
cleanup_system() {
    echo "[INFO] 기존 시스템 정리 중..."
    sudo killall $MQTT_APP 2>/dev/null
    sudo killall $USER_APP 2>/dev/null
    sudo rmmod $DRIVER_NAME 2>/dev/null
    sudo rm -f /dev/$DEVICE_NAME
    sleep 2
    echo "[SUCCESS] 시스템 정리 완료"
}

# 함수: MQTT 환경 설정
setup_mqtt_environment() {
    echo "[INFO] MQTT 환경 설정 중..."
    MQTT_BASE_DIR="$HOME/dev/cpp_libs/qtmqtt/install"
    MQTT_PKGCONFIG_DIR="$MQTT_BASE_DIR/usr/lib/aarch64-linux-gnu/pkgconfig"
    MQTT_LIBDIR="$MQTT_BASE_DIR/usr/lib/aarch64-linux-gnu"

    if [ ! -d "$MQTT_PKGCONFIG_DIR" ]; then
        echo "[WARNING] 사용자 정의 MQTT 패키지를 찾을 수 없습니다."
        echo "[INFO] 시스템 기본 Qt MQTT를 사용합니다."
        return 1
    else
        export PKG_CONFIG_PATH="$MQTT_PKGCONFIG_DIR:$PKG_CONFIG_PATH"
        export LD_LIBRARY_PATH="$MQTT_LIBDIR:$LD_LIBRARY_PATH"
        echo "[SUCCESS] MQTT 환경 설정 완료"
        return 0
    fi
}

# 함수: 빌드
build_system() {
    echo "[INFO] 빌드 시작..."
    make clean > /dev/null 2>&1
    
    if make; then
        echo "[SUCCESS] 빌드 완료"
        return 0
    else
        echo "[ERROR] 빌드 실패"
        return 1
    fi
}

# 함수: 커널 모듈 로드
load_kernel_module() {
    echo "[INFO] 커널 모듈 삽입 중..."
    
    if sudo insmod ${DRIVER_NAME}.ko; then
        echo "[SUCCESS] 모듈 삽입 완료"
        sleep 2
        return 0
    else
        echo "[ERROR] 모듈 삽입 실패"
        return 1
    fi
}

# 함수: 디바이스 파일 생성
create_device_file() {
    echo "[INFO] 디바이스 파일 생성 중..."
    
    # Major 번호 확인
    MAJOR_NUM=$(cat /proc/devices | grep ${DEVICE_NAME} | awk '{print $1}')
    
    if [ -z "$MAJOR_NUM" ]; then
        echo "[ERROR] Major 번호를 찾을 수 없습니다"
        echo "[INFO] /proc/devices 내용:"
        cat /proc/devices | grep -E "(Character|conveyor)"
        return 1
    fi

    echo "[INFO] Major 번호: $MAJOR_NUM"

    # 디바이스 파일 생성
    if sudo mknod /dev/$DEVICE_NAME c $MAJOR_NUM 0; then
        sudo chmod 666 /dev/$DEVICE_NAME
        
        # 생성 확인
        if [ -e "/dev/$DEVICE_NAME" ]; then
            echo "[SUCCESS] 디바이스 파일 생성 완료: /dev/$DEVICE_NAME"
            ls -la /dev/$DEVICE_NAME
            return 0
        else
            echo "[ERROR] 디바이스 파일 생성 실패"
            return 1
        fi
    else
        echo "[ERROR] mknod 명령 실패"
        return 1
    fi
}

# 함수: 시스템 테스트
test_system() {
    echo "[INFO] 시스템 테스트 중..."
    
    # 모듈 로드 확인
    if lsmod | grep -q $DRIVER_NAME; then
        echo "[SUCCESS] 모듈 로드 확인됨"
    else
        echo "[ERROR] 모듈이 로드되지 않음"
        return 1
    fi
    
    # 디바이스 파일 확인
    if [ -e "/dev/$DEVICE_NAME" ]; then
        echo "[SUCCESS] 디바이스 파일 확인됨"
    else
        echo "[ERROR] 디바이스 파일이 없음"
        return 1
    fi
    
    # 직접 테스트
    echo "[INFO] 직접 통신 테스트..."
    if echo "on" > /dev/$DEVICE_NAME 2>/dev/null; then
        echo "[SUCCESS] 디바이스 쓰기 테스트 성공"
        
        if cat /dev/$DEVICE_NAME > /dev/null 2>&1; then
            echo "[SUCCESS] 디바이스 읽기 테스트 성공"
            echo "off" > /dev/$DEVICE_NAME 2>/dev/null
            return 0
        else
            echo "[ERROR] 디바이스 읽기 테스트 실패"
            return 1
        fi
    else
        echo "[ERROR] 디바이스 쓰기 테스트 실패"
        return 1
    fi
}

# 함수: 프로그램 실행
run_program() {
    echo ""
    echo "[INFO] 실행 모드를 선택하세요:"
    echo "1) USER - 사용자 제어 프로그램"
    echo "2) MQTT - MQTT 클라이언트 프로그램"
    echo -n "선택 (1 또는 2): "
    
    read -r MODE_CHOICE
    
    case "$MODE_CHOICE" in
        "1"|"USER")
            echo "[INFO] 사용자 제어 프로그램 실행 중..."
            if [ -x "./$USER_APP" ]; then
                ./$USER_APP
            else
                echo "[ERROR] $USER_APP 실행 파일을 찾을 수 없습니다"
                return 1
            fi
            ;;
        "2"|"MQTT")
            echo "[INFO] MQTT 클라이언트 프로그램 실행 중..."
            echo "[INFO] MQTT 브로커: mqtt.kwon.pics:1883"
            echo "[INFO] 구독 토픽: conveyor/cmd"
            
            if [ -x "./$MQTT_APP" ]; then
                ./$MQTT_APP
            else
                echo "[ERROR] $MQTT_APP 실행 파일을 찾을 수 없습니다"
                return 1
            fi
            ;;
        *)
            echo "[ERROR] 잘못된 선택입니다"
            return 1
            ;;
    esac
}

# 함수: 시스템 정리 질문
cleanup_question() {
    echo ""
    echo "[INFO] 프로그램이 종료되었습니다."
    echo -n "드라이버와 디바이스 파일을 제거하시겠습니까? (y/n): "
    read -r answer

    if [ "$answer" = "y" ] || [ "$answer" = "Y" ]; then
        echo "[INFO] 시스템 정리 중..."
        sudo rmmod $DRIVER_NAME 2>/dev/null
        sudo rm -f /dev/$DEVICE_NAME
        echo "[SUCCESS] 드라이버와 디바이스 파일이 제거되었습니다"
    else
        echo "[INFO] 드라이버는 로드된 상태로 유지됩니다"
        echo "[INFO] 수동 제거: sudo rmmod $DRIVER_NAME && sudo rm -f /dev/$DEVICE_NAME"
    fi
}

# 메인 실행 로직
main() {
    # 1. 시스템 정리
    cleanup_system
    
    # 2. MQTT 환경 설정
    setup_mqtt_environment
    
    # 3. 빌드
    if ! build_system; then
        exit 1
    fi
    
    # 4. 커널 모듈 로드
    if ! load_kernel_module; then
        exit 1
    fi
    
    # 5. 디바이스 파일 생성
    if ! create_device_file; then
        sudo rmmod $DRIVER_NAME 2>/dev/null
        exit 1
    fi
    
    # 6. 시스템 테스트
    if ! test_system; then
        echo "[ERROR] 시스템 테스트 실패"
        cleanup_system
        exit 1
    fi
    
    echo "[SUCCESS] 모든 설정이 완료되었습니다!"
    
    # 7. 프로그램 실행
    run_program
    
    # 8. 정리 질문
    cleanup_question
}

# 스크립트 시작
if [ "$(id -u)" = "0" ]; then
    echo "[ERROR] 이 스크립트는 root 권한으로 실행하지 마세요"
    exit 1
fi

# Ctrl+C 핸들러
trap 'echo ""; echo "[WARNING] 스크립트가 중단되었습니다"; cleanup_system; exit 130' INT

# 메인 함수 실행
main

echo ""
echo "[SUCCESS] === 스크립트 종료 ==="