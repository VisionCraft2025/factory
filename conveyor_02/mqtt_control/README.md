# MQTT 모터 제어 시스템

MQTT 메시지를 구독하여 L298N 모터 드라이버를 제어하는 시스템입니다.

## 📋 목차

- [개요](#개요)
- [시스템 요구사항](#시스템-요구사항)
- [빌드 및 설치](#빌드-및-설치)
- [사용법](#사용법)
- [설정](#설정)
- [문제해결](#문제해결)

## 📖 개요

이 프로그램은 MQTT 브로커에서 메시지를 받아 라즈베리파이의 L298N 커널 드라이버를 통해 모터를 제어합니다. SSL/TLS 연결을 지원하며, 간단한 on/off 명령으로 모터를 제어할 수 있습니다.

### 주요 기능
- MQTT over SSL/TLS 지원
- L298N 커널 드라이버와 연동
- 간단한 on/off 명령으로 모터 제어
- 안전한 종료 처리 (Ctrl+C)
- 실시간 상태 표시

## 🔧 시스템 요구사항

- 라즈베리파이 (Linux)
- L298N 모터 드라이버 커널 모듈 (../motor_driver/)
- paho.mqtt.c 라이브러리 (설치된 상태)
- OpenSSL 라이브러리
- CA 인증서 파일 (ca.crt)

## 💾 빌드 및 설치

### 전제 조건
1. paho.mqtt.c 라이브러리가 `~/dev/cpp_libs/paho.mqtt.c`에 설치되어 있어야 합니다.
2. L298N 커널 모듈이 로드되어 있어야 합니다.

### 빌드 과정
```bash
# 빌드 디렉토리 생성 및 이동
mkdir build && cd build

# CMake 설정
cmake ..

# 컴파일
make

# CA 인증서 준비
cp /path/to/your/ca.crt .
```

### 빌드 결과
```
build/
├── mqtt_motor_control      # 실행 파일
└── ca.crt                 # SSL 인증서
```

## 🚀 사용법

### 기본 실행
```bash
# 기본 토픽 (conveyor02/cmd)으로 실행
sudo ./mqtt_motor_control

# 사용자 정의 토픽으로 실행
sudo ./mqtt_motor_control "your/custom/topic"
```

**⚠️ 주의**: 디바이스 파일 접근을 위해 sudo 권한이 필요합니다.

### MQTT 명령어

프로그램은 지정된 토픽에서 다음 페이로드를 인식합니다:

| 페이로드 | 동작 | 설명 |
|---------|------|------|
| `on` | 모터 켜짐 | 모터A 정방향 99% 속도 |
| `off` | 모터 끄짐 | 모든 모터 정지 |

* 대소문자 구분 없음 (ON, Off, oN 등 모두 인식)

### MQTT 메시지 전송 예시

```bash
# 모터 켜기
mosquitto_pub -h mqtt.kwon.pics -p 8883 --cafile ca.crt -t "conveyor02/cmd" -m "on"

# 모터 끄기
mosquitto_pub -h mqtt.kwon.pics -p 8883 --cafile ca.crt -t "conveyor02/cmd" -m "off"

# 대소문자 상관없음
mosquitto_pub -h mqtt.kwon.pics -p 8883 --cafile ca.crt -t "conveyor02/cmd" -m "ON"
mosquitto_pub -h mqtt.kwon.pics -p 8883 --cafile ca.crt -t "conveyor02/cmd" -m "OFF"
```

### 실행 화면 예시
```
=== MQTT 모터 제어 시스템 시작 ===
구독 토픽: conveyor02/cmd
Ctrl+C로 안전하게 종료할 수 있습니다.
페이로드 명령어:
  on       - 모터A 정방향 99% (켜짐)
  off      - 모터 정지 (꺼짐)

Connecting to broker: ssl://mqtt.kwon.pics:8883
Using CA certificate: ./ca.crt
✓ MQTT 브로커에 연결되었습니다!
✓ 토픽 구독: conveyor02/cmd
```

## ⚙️ 설정

### 기본 설정값

```cpp
std::string broker_url = "ssl://mqtt.kwon.pics:8883";
std::string client_id = "RaspberryPiMotorController";
std::string ca_cert_path = "./ca.crt";
std::string subscribe_topic = "conveyor02/cmd";  // 또는 명령행 인자
```

### 설정 변경

코드 내에서 다음 변수들을 수정하여 설정을 변경할 수 있습니다:

1. **브로커 주소 변경**
   ```cpp
   std::string broker_url = "ssl://your-broker.com:8883";
   ```

2. **클라이언트 ID 변경**
   ```cpp
   std::string client_id = "YourCustomClientID";
   ```

3. **모터 속도 변경** (on 명령 시)
   ```cpp
   // process_motor_command 함수에서
   send_motor_command('A', 1, 80);  // 99 대신 80% 속도
   ```

### 토픽 설정

- **기본 토픽**: `conveyor02/cmd`
- **명령행 인자로 변경**: `./mqtt_motor_control "new/topic"`

## 🔧 문제해결

### SSL 연결 문제

#### 1. SSL 연결 실패
```bash
# 네트워크 연결 확인
ping mqtt.kwon.pics

# SSL 포트 확인
nmap -p 8883 mqtt.kwon.pics

# 방화벽 확인
sudo ufw status
```

#### 2. 인증서 파일 없음
```bash
# 오류 메시지
CA certificate file not found: ./ca.crt

# 해결방법
ls -la ca.crt  # 파일 존재 확인
cp /path/to/correct/ca.crt .  # 올바른 위치에서 복사
```

#### 3. 인증서 검증 실패
임시로 검증을 비활성화하여 테스트:
```cpp
// connect() 함수에서
ssl_opts.verify = 0;                // 인증서 검증 비활성화
ssl_opts.enableServerCertAuth = 0;  // 서버 인증 비활성화
```

### 모터 제어 문제

#### 1. 디바이스 파일 접근 오류
```bash
# 오류 메시지
디바이스가 열려있지 않습니다.

# 해결방법
ls -l /dev/l298n_motor  # 디바이스 파일 확인
sudo chmod 666 /dev/l298n_motor  # 권한 설정

# 커널 모듈 로드 확인
cd ../motor_driver
make status
```

#### 2. 권한 문제
```bash
# sudo 권한으로 실행
sudo ./mqtt_motor_control

# 사용자를 적절한 그룹에 추가
sudo usermod -a -G gpio $USER
```

### MQTT 메시지 문제

#### 1. 메시지가 도착하지 않음
```bash
# 구독 테스트
mosquitto_sub -h mqtt.kwon.pics -p 8883 --cafile ca.crt -t "conveyor02/cmd"

# 발행 테스트
mosquitto_pub -h mqtt.kwon.pics -p 8883 --cafile ca.crt -t "conveyor02/cmd" -m "test"
```

#### 2. 토픽 불일치
프로그램이 구독하는 토픽과 발행하는 토픽이 정확히 일치하는지 확인:
```bash
# 프로그램 실행 시 표시되는 토픽 확인
구독 토픽: conveyor02/cmd

# 동일한 토픽으로 메시지 발행
mosquitto_pub ... -t "conveyor02/cmd" -m "on"
```

### 컴파일 문제

#### 1. paho.mqtt.c 라이브러리 경로 오류
```bash
# CMakeLists.txt에서 경로 확인
set(PAHO_MQTT_C_DIR "/home/kwon/dev/cpp_libs/paho.mqtt.c")

# 실제 라이브러리 위치 확인
ls -la ~/dev/cpp_libs/paho.mqtt.c/build/src/
```

#### 2. 헤더 파일을 찾을 수 없음
```bash
# 헤더 파일 위치 확인
find ~/dev/cpp_libs/paho.mqtt.c -name "MQTTClient.h"

# 라이브러리 파일 확인
ls ~/dev/cpp_libs/paho.mqtt.c/build/src/libpaho-mqtt3cs.so
```

### 디버깅 방법

#### 1. 실시간 로그 확인
```bash
# 프로그램 실행과 동시에 다른 터미널에서
sudo dmesg -w | grep "L298N"
```

#### 2. MQTT 연결 상태 확인
```bash
# 연결 테스트
mosquitto_pub -h mqtt.kwon.pics -p 8883 --cafile ca.crt -t "test" -m "hello"
```

#### 3. 단계별 테스트
```bash
# 1단계: 모터 드라이버 직접 테스트
echo "A 1 50" | sudo tee /dev/l298n_motor

# 2단계: MQTT 프로그램 연결만 테스트
./mqtt_motor_control test/topic  # 존재하지 않는 토픽으로 연결 테스트

# 3단계: 실제 메시지 테스트
mosquitto_pub -h mqtt.kwon.pics -p 8883 --cafile ca.crt -t "conveyor02/cmd" -m "on"
```

### 로그 해석

#### 정상 동작 로그
```
Connected to MQTT broker successfully!
✓ 토픽 구독: conveyor02/cmd
토픽 'conveyor02/cmd'에서 메시지 도착: on
모터 켜짐 - 모터A 정방향 속도: 99%
✓ 모터 명령 실행: A 1 99
```

#### 연결 실패 로그
```
Failed to connect, return code: -1
Connection failed!
```

#### 인증서 오류 로그
```
CA certificate file not found: ./ca.crt
Failed to connect, return code: -1
```

## 📁 파일 구조

```
mqtt_control/
├── mqtt_motor_control.cpp    # 메인 소스 코드
├── CMakeLists.txt           # CMake 빌드 설정
├── README.md               # 이 문서
└── build/                  # 빌드 디렉토리
    ├── mqtt_motor_control  # 실행 파일
    └── ca.crt             # CA 인증서
```

## 🛡️ 안전 기능

### 시그널 핸들러
프로그램 종료 시 모든 모터를 안전하게 정지합니다:
```cpp
// Ctrl+C 또는 종료 시그널 시
void signal_handler(int sig) {
    controller->stop();        // 모터 정지
    controller->cleanup();     // 리소스 정리
    exit(0);
}
```

### 오류 처리
- SSL 연결 실패 시 적절한 오류 메시지 출력
- 디바이스 파일 오류 시 안전한 종료
- 잘못된 명령어 무시

### 상태 표시
실시간으로 다음 정보를 표시합니다:
- MQTT 연결 상태
- 구독 토픽 정보
- 받은 메시지 내용
- 모터 제어 결과

## 💡 사용 팁

### 1. 자동 시작 설정
시스템 부팅 시 자동으로 실행하려면:
```bash
# systemd 서비스 파일 생성
sudo nano /etc/systemd/system/mqtt-motor.service
```

```ini
[Unit]
Description=MQTT Motor Control
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=/home/pi/mqtt_control/build
ExecStart=/home/pi/mqtt_control/build/mqtt_motor_control
Restart=always

[Install]
WantedBy=multi-user.target
```

```bash
# 서비스 활성화
sudo systemctl enable mqtt-motor.service
sudo systemctl start mqtt-motor.service
```

### 2. 로그 파일 생성
실행 로그를 파일로 저장:
```bash
sudo ./mqtt_motor_control > /var/log/mqtt-motor.log 2>&1 &
```

### 3. 원격 디버깅
SSH를 통한 원격 제어:
```bash
# SSH 연결 후
cd /path/to/mqtt_control/build
sudo ./mqtt_motor_control

# 다른 터미널에서 테스트
mosquitto_pub -h mqtt.kwon.pics -p 8883 --cafile ca.crt -t "conveyor02/cmd" -m "on"
```