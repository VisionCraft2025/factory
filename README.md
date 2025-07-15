<<<<<<< HEAD
# 라즈베리파이 모터 제어 시스템

L298N 모터 드라이버를 사용한 라즈베리파이 DC 모터 제어 시스템입니다. 커널 모듈과 MQTT 제어 인터페이스를 제공합니다.

## 🏗️ 시스템 구성

```
모터 제어 시스템
├── motor_driver/           # L298N 커널 드라이버
│   ├── l298n_motor_driver.c
│   ├── Makefile
│   ├── README.md          # 커널 드라이버 상세 문서
│   └── build/             # 빌드 결과물
└── mqtt_control/          # MQTT 기반 제어 시스템
    ├── mqtt_motor_control.cpp
    ├── CMakeLists.txt
    ├── README.md          # MQTT 제어 상세 문서
    └── build/             # 빌드 결과물
        └── ca.crt         # SSL 인증서
```

## 🔧 하드웨어 요구사항

- 라즈베리파이 (모든 모델)
- L298N 모터 드라이버 모듈
- DC 모터 1~2개
- 점퍼 와이어
- 외부 전원 (7-35V, 모터에 따라)

## 🔌 하드웨어 연결

### 기본 연결 (PWM 속도 제어)
```
라즈베리파이 GPIO    →    L298N
GPIO 18 (PWM)       →    ENA (점퍼 제거)
GPIO 23             →    IN1
GPIO 24             →    IN2
GPIO 25             →    IN3
GPIO 8              →    IN4
GPIO 7 (PWM)        →    ENB (점퍼 제거)
GND                 →    GND
```

### 전원 연결
```
외부 전원 (+)       →    L298N 12V
외부 전원 (-)       →    L298N GND
라즈베리파이 GND    →    L298N GND (공통 접지 필수)
```

**⚠️ 중요**: ENA, ENB 점퍼를 제거하고 GPIO에 연결해야 PWM 속도 제어가 가능합니다.

## 💾 설치 및 준비

### 1. 개발 환경 준비
```bash
# 라즈베리파이 OS 업데이트
sudo apt update && sudo apt upgrade -y

# 필요한 패키지 설치
sudo apt install raspberrypi-kernel-headers build-essential cmake git libssl-dev
```

### 2. paho.mqtt.c 라이브러리 설치
```bash
# 라이브러리 디렉토리 생성
mkdir -p ~/dev/cpp_libs
cd ~/dev/cpp_libs

# paho.mqtt.c 클론 및 빌드
git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.c
mkdir build && cd build

# CMake 설정 및 빌드
cmake .. -DPAHO_ENABLE_TESTING=OFF -DPAHO_BUILD_DOCUMENTATION=OFF -DPAHO_WITH_SSL=ON
make -j$(nproc)
```

### 3. SSL 인증서 준비
MQTT over SSL 연결을 위해 CA 인증서 파일이 필요합니다:
```bash
# 인증서를 mqtt_control/build/ 디렉토리에 복사
cp /path/to/your/ca.crt mqtt_control/build/
```

## 🚀 빠른 시작

### 1. 모터 드라이버 설치
```bash
cd motor_driver
make install
```

### 2. MQTT 제어 프로그램 빌드
```bash
cd mqtt_control
mkdir build && cd build
cmake ..
make
```

### 3. 실행
```bash
# 기본 토픽(conveyor02/cmd)으로 MQTT 제어 시작
sudo ./mqtt_motor_control
```

### 4. 모터 제어 테스트
```bash
# MQTT 메시지로 모터 제어
mosquitto_pub -h mqtt.kwon.pics -p 8883 --cafile ca.crt -t "conveyor02/cmd" -m "on"
mosquitto_pub -h mqtt.kwon.pics -p 8883 --cafile ca.crt -t "conveyor02/cmd" -m "off"
```

## 📁 상세 문서

각 구성 요소의 상세한 사용법은 해당 폴더의 README를 참조하세요:

- **[motor_driver/README.md](motor_driver/README.md)**: L298N 커널 드라이버 상세 문서
- **[mqtt_control/README.md](mqtt_control/README.md)**: MQTT 제어 시스템 상세 문서

## 🔧 기본 문제해결

### 권한 오류
```bash
# 디바이스 파일 권한 설정
sudo chmod 666 /dev/l298n_motor

# MQTT 프로그램은 sudo로 실행
sudo ./mqtt_motor_control
=======
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
>>>>>>> conveyor_mqtt
```

### 모듈 로드 실패
```bash
<<<<<<< HEAD
# 기존 모듈 제거 후 재설치
cd motor_driver
make reload
```

### MQTT 연결 실패
```bash
# 인증서 파일 확인
ls -la mqtt_control/build/ca.crt

# 네트워크 연결 확인
ping mqtt.kwon.pics
```
# MQTT Control System

4축 서보모터 기반 로봇팔/모터기반 feeder를 MQTT 프로토콜로 원격 제어하는 시스템입니다.

---

## 하드웨어 구성

### robot arm

| 서보모터 | GPIO 핀 | 기능 |
|---------|---------|------|
| servo0 | 22 | 하단 모터 |
| servo1 | 24 | 중단 모터 |
| servo2 | 25 | 상단 모터 |
| servo3 | 8 | 엔드 모터(그리퍼) |

### feeder

feeder 핀 번호: 5, 6, 13, 19

---

## 설치 가이드

### Step 1: Qt6 MQTT 설정
통합 mqtt 라이브러리 설치 경로를 사용합니다.

```bash
~/dev
```

### Step 2: 프로젝트 실행

#### feeder
```bash
sudo ./setup_feeder.sh
```

#### robot arm
```bash
sudo ./setup_robot_arm.sh 
```

---

## 제어 모드


### robot arm

#### Manual Mode(USER)
```
실행 후 "1" 선택
> auto_on      ← 자동 시퀀스 실행(재활용 안되는 거 받아서 버리기)
> auto off     ← llow 종료, 초기값으로 복귀 
> servo0 90    ← 베이스를 90도로 회전
> init         ← 초기 위치로 복귀
> exit         ← 프로그램 종료
```

### Feeder
```
on        # 정방향 회전
reverse   # 역방향 회전
off       # 회전 정지
error     # 에러모드, 30초간 정상 작동 후 점차 느려짐.(멈추진 않음)
normal    # 정상속도 복귀
exit      # 종료
```

###  MQTT Mode (원격 제어)
```
실행 후 "2" 선택
토픽: robot_arm/cmd
브로커: mqtt.kwon.pics:1883
```

---

## MQTT 명령어 레퍼런스

### robot arm
#### 기본 제어
```bash
# 시스템 제어
mosquitto_pub -h mqtt.kwon.pics -t robot_arm/cmd -m "init"      # 초기화
mosquitto_pub -h mqtt.kwon.pics -t robot_arm/cmd -m "auto_on"   # 자동 시작
mosquitto_pub -h mqtt.kwon.pics -t robot_arm/cmd -m "auto_off"  # 자동 중지
```

#### 위치 제어
```bash
# 수동 제어 (0-250도)
mosquitto_pub -h mqtt.kwon.pics -t robot_arm/cmd -m "servo0 45"
mosquitto_pub -h mqtt.kwon.pics -t robot_arm/cmd -m "servo1 120"
mosquitto_pub -h mqtt.kwon.pics -t robot_arm/cmd -m "servo2 180"
mosquitto_pub -h mqtt.kwon.pics -t robot_arm/cmd -m "servo3 90"
```


### Feeder

"feeder/cmd" 토픽을 구독

토픽으로 명령어 전송 시 자동으로 /dev/feeder에 write

```bash
mosquitto_pub -h mqtt.kwon.pics -p 1883 -t feeder/cmd -m "on"
mosquitto_pub -h mqtt.kwon.pics -p 1883 -t feeder/cmd -m "reverse"
mosquitto_pub -h mqtt.kwon.pics -p 1883 -t feeder/cmd -m "off"
mosquitto_pub -h mqtt.kwon.pics -p 1883 -t feeder/cmd -m "error"
mosquitto_pub -h mqtt.kwon.pics -p 1883 -t feeder/cmd -m "normal"
```
---
=======
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
>>>>>>> conveyor_mqtt
