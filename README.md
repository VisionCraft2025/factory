# Conveyor Control System

스마트 팩토리용 컨베이어 제어 시스템 - 스테퍼 모터와 서보 모터를 이용한 자동화 컨베이어 벨트 제어

## 📋 목차
- [개요](#개요)
- [시스템 구성](#시스템-구성)
- [하드웨어 요구사항](#하드웨어-요구사항)
- [설치 방법](#설치-방법)
- [사용법](#사용법)
- [파일 구성](#파일-구성)
- [기능 설명](#기능-설명)
- [문제 해결](#문제-해결)

## 🎯 개요

이 프로젝트는 라즈베리파이를 기반으로 한 스마트 팩토리 컨베이어 벨트 제어 시스템입니다. 
- **스테퍼 모터**: 정밀한 컨베이어 벨트 이동 제어
- **서보 모터**: 제품 밀어내기 자동화
- **MQTT 통신**: 원격 제어 및 모니터링
- **에러 모드**: 속도 저하 시뮬레이션으로 이상 상황 감지

## 🔧 시스템 구성

```
[MQTT 브로커] ←→ [Qt MQTT Client] ←→ [Kernel Driver] ←→ [Hardware]
                     ↕
              [User Controller]
```

### 핵심 컴포넌트
1. **Kernel Driver**: 하드웨어 직접 제어 (GPIO, PWM)
2. **MQTT Client**: 원격 명령 수신 및 디바이스 제어
3. **User Controller**: 로컬 사용자 인터페이스
4. **Automation System**: 자동 판 감지 및 서보 제어

## 🛠️ 하드웨어 요구사항

### 라즈베리파이 GPIO 연결
| 컴포넌트 | GPIO 핀 | BCM 번호 | 설명 |
|----------|---------|----------|------|
| 스테퍼 모터 STEP | GPIO 17 | BCM 17 | 스텝 펄스 신호 |
| 스테퍼 모터 DIR | GPIO 27 | BCM 27 | 방향 제어 |
| 스테퍼 모터 ENABLE | GPIO 22 | BCM 22 | 모터 활성화 |
| 서보 모터 PWM | GPIO 5 | BCM 5 | 서보 각도 제어 |

### 필요 하드웨어
- 라즈베리파이 4B+ (권장)
- 스테퍼 모터 + 드라이버 (A4988/DRV8825)
- 서보 모터 (SG90 등)
- 컨베이어 벨트 메카니즘
- 전원 공급 장치

## 🚀 설치 방법

### 1. 자동 설치 (권장)
```bash
# 저장소 클론
git clone https://github.com/VisionCraft2025/factory.git
cd factory/conveyor_mqtt

# 자동 설정 스크립트 실행
chmod +x setup_conveyor.sh
./setup_conveyor.sh
```

### 2. 수동 설치
```bash
# 빌드
make all

# 모듈 설치
make install

# 테스트
make test

# MQTT 클라이언트 실행
make mqtt
```

### 3. 의존성 설치
```bash
# Qt6 개발 도구
sudo apt update
sudo apt install qt6-base-dev qt6-tools-dev build-essential

# 커널 헤더
sudo apt install linux-headers-$(uname -r)

# Git
sudo apt install git
```

## 📖 사용법

### MQTT 원격 제어
```bash
# MQTT 클라이언트 실행
./conveyor_mqtt

# 다른 터미널에서 명령 전송
mosquitto_pub -h mqtt.kwon.pics -t "conveyor/cmd" -m "on"
mosquitto_pub -h mqtt.kwon.pics -t "conveyor/cmd" -m "error_mode"
mosquitto_pub -h mqtt.kwon.pics -t "conveyor/cmd" -m "off"
```

### 로컬 제어
```bash
# 사용자 컨트롤러 실행
./conveyor_user

# 명령어 입력
명령어 입력> on          # 컨베이어 시작
명령어 입력> error_mode  # 에러 모드 (30초 후 속도 감소)
명령어 입력> status      # 현재 상태 확인
명령어 입력> off         # 컨베이어 정지
명령어 입력> exit        # 프로그램 종료
```

### 직접 제어
```bash
# 디바이스 파일 직접 조작
echo "on" > /dev/conveyor_mqtt
cat /dev/conveyor_mqtt  # 상태 확인
echo "off" > /dev/conveyor_mqtt
```

## 📁 파일 구성

```
conveyor_mqtt/
├── conveyor_driver.c     # 커널 드라이버 (GPIO 제어)
├── conveyor_mqtt.cpp     # MQTT 클라이언트
├── conveyor_user.cpp     # 사용자 제어 프로그램
├── Makefile             # 빌드 스크립트
├── setup_conveyor.sh    # 자동 설정 스크립트
└── README.md           # 이 파일
```

### 주요 파일 설명

#### `conveyor_driver.c`
- 리눅스 커널 모듈
- GPIO 직접 제어 (스테퍼/서보 모터)
- Character Device 인터페이스 (/dev/conveyor_mqtt)
- 자동 판 감지 및 서보 제어
- 에러 모드 시뮬레이션

#### `conveyor_mqtt.cpp`
- Qt6 기반 MQTT 클라이언트
- 브로커: mqtt.kwon.pics:1883
- 토픽: conveyor/cmd
- 실시간 원격 제어

#### `conveyor_user.cpp`
- C++ 기반 로컬 제어 프로그램
- 대화형 사용자 인터페이스
- 직접 디바이스 파일 조작

## ⚙️ 기능 설명

### 1. 자동 컨베이어 제어
- **첫 번째 판**: 288 스텝 (6.5cm)
- **나머지 판**: 392 스텝 (8.5cm) 
- **총 10칸** 자동 순환

### 2. 서보 자동화
- 각 판 도달 시 자동으로 180도 회전 (밀어내기)
- 1초 후 자동으로 0도 복귀
- PWM 신호 기반 정밀 제어

### 3. 에러 모드 시뮬레이션
```
정상 작동 (30초) → 속도 감소 1차 (100→90) → 속도 감소 2차 (90→80) → 고정
```

### 4. 실시간 상태 모니터링
- 컨베이어 ON/OFF 상태
- 서보 각도 (현재/목표)
- 스테퍼 방향
- 현재 속도
- 스텝 카운터 및 판 위치

## 🔧 문제 해결

### 권한 오류
```bash
# 디바이스 파일 권한 확인
ls -la /dev/conveyor_mqtt

# 권한 수정
sudo chmod 666 /dev/conveyor_mqtt
```

### 모듈 로드 실패
```bash
# 커널 로그 확인
dmesg | tail -10

# 모듈 상태 확인
lsmod | grep conveyor

# 수동 모듈 제거
sudo rmmod conveyor_driver
```

### Major 번호 문제
```bash
# Major 번호 확인
cat /proc/devices | grep conveyor_mqtt

# 수동 디바이스 파일 생성
sudo mknod /dev/conveyor_mqtt c [MAJOR_NUMBER] 0
sudo chmod 666 /dev/conveyor_mqtt
```

### MQTT 연결 문제
```bash
# 네트워크 연결 확인
ping mqtt.kwon.pics

# Qt6 MQTT 라이브러리 확인
ldd ./conveyor_mqtt

# 환경 변수 설정
export LD_LIBRARY_PATH=/home/veda/dev/cpp_libs/qtmqtt/install/usr/lib/aarch64-linux-gnu:$LD_LIBRARY_PATH
```

### 빌드 오류
```bash
# 의존성 재설치
sudo apt install --reinstall qt6-base-dev linux-headers-$(uname -r)

# 클린 빌드
make clean
make all
```
