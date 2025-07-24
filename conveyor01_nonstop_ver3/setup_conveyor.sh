#!/bin/bash
DRIVER_NAME=conveyor_driver
DEVICE_NAME=conveyor_mqtt
USER_APP=conveyor_user
MQTT_APP=conveyor_mqtt
MQTT_TLS_APP=conveyor_mqtt_tls

echo "=== Enhanced Conveyor System Setup ==="

# 강화된 정리 함수
cleanup_system() {
    echo "[INFO] 강화된 시스템 정리 중..."
    
    # 1. 프로세스 강제 종료
    sudo killall -9 $MQTT_TLS_APP 2>/dev/null
    sudo killall -9 $MQTT_APP 2>/dev/null
    sudo killall -9 $USER_APP 2>/dev/null
    
    # 2. 커널 스레드 정리
    STEPPER_PID=$(ps -eLf | grep stepper_motor | grep -v grep | awk '{print $2}')
    SERVO_PID=$(ps -eLf | grep servo_motor | grep -v grep | awk '{print $2}')
    
    [ ! -z "$STEPPER_PID" ] && sudo kill -9 $STEPPER_PID 2>/dev/null
    [ ! -z "$SERVO_PID" ] && sudo kill -9 $SERVO_PID 2>/dev/null
    
    # 3. 디바이스 파일 제거
    sudo rm -f /dev/$DEVICE_NAME
    
    # 4. 모듈 제거 (여러 번 시도)
    for i in {1..3}; do
        if lsmod | grep -q $DRIVER_NAME; then
            echo "[INFO] 모듈 제거 시도 $i/3..."
            sudo rmmod $DRIVER_NAME 2>/dev/null
            sleep 1
        else
            break
        fi
    done
    
    # 5. 강제 모듈 제거 (마지막 수단)
    if lsmod | grep -q $DRIVER_NAME; then
        echo "[WARNING] 강제 모듈 제거 시도..."
        sudo rmmod -f $DRIVER_NAME 2>/dev/null
        sleep 2
    fi
    
    # 6. 최종 확인
    if lsmod | grep -q $DRIVER_NAME; then
        echo "[ERROR] 모듈 제거 실패! 재부팅이 필요합니다."
        echo "sudo reboot를 실행하세요."
        exit 1
    fi
    
    # 7. 리소스 정리
    sync
    echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null
    
    sleep 2
    echo "[SUCCESS] 강화된 시스템 정리 완료"
}

# 리소스 상태 확인 함수
check_resources() {
    echo "[INFO] 리소스 상태 확인 중..."
    
    # GPIO 상태 확인
    if [ -r "/sys/kernel/debug/gpio" ]; then
        GPIO_CHECK=$(sudo cat /sys/kernel/debug/gpio | grep -E "gpio-17|gpio-22|gpio-27|gpio-5")
        if [ ! -z "$GPIO_CHECK" ]; then
            echo "[WARNING] GPIO 핀이 점유되어 있을 수 있습니다:"
            echo "$GPIO_CHECK"
        fi
    fi
    
    # Character Device 확인
    CHAR_DEV=$(cat /proc/devices | grep $DEVICE_NAME)
    if [ ! -z "$CHAR_DEV" ]; then
        echo "[WARNING] Character Device가 이미 등록되어 있습니다: $CHAR_DEV"
        return 1
    fi
    
    return 0
}

# MQTT 환경 설정
setup_mqtt_environment() {
    echo "[INFO] MQTT 환경 설정 중..."
    MQTT_BASE_DIR="$HOME/dev/cpp_libs/qtmqtt/install"
    MQTT_PKGCONFIG_DIR="$MQTT_BASE_DIR/usr/lib/aarch64-linux-gnu/pkgconfig"
    MQTT_LIBDIR="$MQTT_BASE_DIR/usr/lib/aarch64-linux-gnu"

    TLS_MQTT_PKGCONFIG_DIR=$(find $HOME/tls_mqtt/qt6_local -type d -name pkgconfig | grep aarch64 | head -n 1)
    TLS_MQTT_LIBDIR=$(find $HOME/tls_mqtt/qt6_local -type f -name "libQt6Mqtt.so*" | xargs dirname | head -n 1)

    if [ ! -d "$MQTT_PKGCONFIG_DIR" ] && [ -z "$TLS_MQTT_PKGCONFIG_DIR" ]; then
        echo "[WARNING] MQTT 패키지를 찾을 수 없습니다."
        return 1
    else
        if [ -n "$TLS_MQTT_PKGCONFIG_DIR" ]; then
            export PKG_CONFIG_PATH="$TLS_MQTT_PKGCONFIG_DIR:$PKG_CONFIG_PATH"
            export LD_LIBRARY_PATH="$TLS_MQTT_LIBDIR:$LD_LIBRARY_PATH"
            echo "[SUCCESS] TLS MQTT 환경 설정 완료"
        else
            export PKG_CONFIG_PATH="$MQTT_PKGCONFIG_DIR:$PKG_CONFIG_PATH"
            export LD_LIBRARY_PATH="$MQTT_LIBDIR:$LD_LIBRARY_PATH"
            echo "[SUCCESS] 기본 MQTT 환경 설정 완료"
        fi
        return 0
    fi
}

# 빌드
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

# 안전한 모듈 로드
load_kernel_module() {
    echo "[INFO] 커널 모듈 삽입 중..."
    
    # 최종 리소스 체크
    if ! check_resources; then
        echo "[ERROR] 리소스가 정리되지 않았습니다. 강제 정리를 시도합니다."
        cleanup_system
        
        # 재검사
        if ! check_resources; then
            echo "[ERROR] 리소스 정리 실패. 재부팅이 필요합니다."
            echo "sudo reboot를 실행하세요."
            exit 1
        fi
    fi
    
    # 모듈 삽입 시도
    if sudo insmod ${DRIVER_NAME}.ko; then
        echo "[SUCCESS] 모듈 삽입 완료"
        sleep 2
        return 0
    else
        echo "[ERROR] 모듈 삽입 실패"
        
        # 에러 상세 분석
        echo "[INFO] 에러 분석 중..."
        dmesg | tail -5
        
        return 1
    fi
}

# 디바이스 파일 생성
create_device_file() {
    echo "[INFO] 디바이스 파일 생성 중..."
    
    MAJOR_NUM=$(cat /proc/devices | grep ${DEVICE_NAME} | awk '{print $1}')
    
    if [ -z "$MAJOR_NUM" ]; then
        echo "[ERROR] Major 번호를 찾을 수 없습니다"
        return 1
    fi

    if sudo mknod /dev/$DEVICE_NAME c $MAJOR_NUM 0; then
        sudo chmod 666 /dev/$DEVICE_NAME
        
        if [ -e "/dev/$DEVICE_NAME" ]; then
            echo "[SUCCESS] 디바이스 파일 생성 완료: /dev/$DEVICE_NAME"
            return 0
        fi
    fi
    
    echo "[ERROR] 디바이스 파일 생성 실패"
    return 1
}

# 시스템 테스트
test_system() {
    echo "[INFO] 시스템 테스트 중..."
    
    if lsmod | grep -q $DRIVER_NAME && [ -e "/dev/$DEVICE_NAME" ]; then
        if echo "on" > /dev/$DEVICE_NAME 2>/dev/null; then
            echo "[SUCCESS] 시스템 테스트 완료"
            echo "off" > /dev/$DEVICE_NAME 2>/dev/null
            return 0
        fi
    fi
    
    echo "[ERROR] 시스템 테스트 실패"
    return 1
}

# 프로그램 실행
run_program() {
    echo ""
    echo "[INFO] 실행 모드를 선택하세요:"
    echo "1) USER - 사용자 제어 프로그램"
    echo "2) MQTT - MQTT 클라이언트 프로그램 (비암호화)"
    echo "3) MQTT-TLS - MQTT 클라이언트 프로그램 (TLS 암호화)"
    echo -n "선택 (1, 2 또는 3): "
    
    read -r MODE_CHOICE
    
    case "$MODE_CHOICE" in
        "1"|"USER")
            if [ -x "./$USER_APP" ]; then
                ./$USER_APP
            else
                echo "[ERROR] $USER_APP 실행 파일을 찾을 수 없습니다"
                return 1
            fi
            ;;
        "2"|"MQTT")
            if [ -x "./$MQTT_APP" ]; then
                ./$MQTT_APP
            else
                echo "[ERROR] $MQTT_APP 실행 파일을 찾을 수 없습니다"
                return 1
            fi
            ;;
        "3"|"MQTT-TLS")
            # 인증서 확인 로직 (기존과 동일)
            echo "[INFO] MQTT TLS 클라이언트 프로그램 실행 중..."
            
            if [ ! -f "/certs/ca.crt" ] && [ -f "$HOME/certs/ca.crt" ]; then
                export CERT_PATH="$HOME/certs"
            elif [ ! -f "/certs/ca.crt" ] && [ -f "./certs/ca.crt" ]; then
                export CERT_PATH="./certs"
            else
                export CERT_PATH="/certs"
            fi
            
            if [ ! -f "$CERT_PATH/ca.crt" ] || [ ! -f "$CERT_PATH/conveyor_03.crt" ] || [ ! -f "$CERT_PATH/conveyor_03.key" ]; then
                echo "[ERROR] 인증서 파일이 없습니다."
                return 1
            fi
            
            if [ -x "./$MQTT_TLS_APP" ]; then
                CERT_PATH="$CERT_PATH" ./$MQTT_TLS_APP
            else
                echo "[ERROR] $MQTT_TLS_APP 실행 파일을 찾을 수 없습니다"
                return 1
            fi
            ;;
        *)
            echo "[ERROR] 잘못된 선택입니다"
            return 1
            ;;
    esac
}

# 정리 질문
cleanup_question() {
    echo ""
    echo "[INFO] 프로그램이 종료되었습니다."
    echo -n "드라이버와 디바이스 파일을 제거하시겠습니까? (y/n): "
    read -r answer

    if [ "$answer" = "y" ] || [ "$answer" = "Y" ]; then
        cleanup_system
    else
        echo "[INFO] 드라이버는 로드된 상태로 유지됩니다"
    fi
}

# 메인 실행 로직
main() {
    # 1. 강화된 시스템 정리
    cleanup_system
    
    # 2. 리소스 상태 최종 확인
    if ! check_resources; then
        echo "[ERROR] 리소스 정리 실패. 재부팅이 필요합니다."
        echo "sudo reboot를 실행하세요."
        exit 1
    fi
    
    # 3. MQTT 환경 설정
    setup_mqtt_environment
    
    # 4. 빌드
    if ! build_system; then
        exit 1
    fi
    
    # 5. 커널 모듈 로드
    if ! load_kernel_module; then
        exit 1
    fi
    
    # 6. 디바이스 파일 생성
    if ! create_device_file; then
        cleanup_system
        exit 1
    fi
    
    # 7. 시스템 테스트
    if ! test_system; then
        cleanup_system
        exit 1
    fi
    
    echo "[SUCCESS] 모든 설정이 완료되었습니다!"
    
    # 8. 프로그램 실행
    run_program
    
    # 9. 정리 질문
    cleanup_question
}

# 권한 체크
if [ "$(id -u)" = "0" ]; then
    echo "[ERROR] 이 스크립트는 root 권한으로 실행하지 마세요"
    exit 1
fi

# Ctrl+C 핸들러
trap 'echo ""; echo "[WARNING] 스크립트가 중단되었습니다"; cleanup_system; exit 130' INT

# 메인 함수 실행
main

echo "[SUCCESS] === 스크립트 종료 ==="