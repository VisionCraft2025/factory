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
```

### 모듈 로드 실패
```bash
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