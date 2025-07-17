# TLS MQTT 컨베이어 제어 시스템

이 프로젝트는 MQTT 프로토콜과 TLS 보안을 사용하여 컨베이어 모터를 제어하는 시스템입니다.
기존의 mqtt_motor_control을 현대적인 방식으로 개선하여 보안성과 확장성을 높였습니다.

## 📋 목차

- [개요](#개요)
- [주요 기능](#주요-기능)
- [시스템 요구사항](#시스템-요구사항)
- [빌드 및 설치](#빌드-및-설치)
- [실행 방법](#실행-방법)
- [토픽 구조](#토픽-구조)
- [명령어](#명령어)
- [인증서 설정](#인증서-설정)
- [문제해결](#문제해결)
- [사용 팁](#사용-팁)

## 📖 개요

이 프로그램은 MQTT 브로커에서 메시지를 받아 라즈베리파이의 L298N 커널 드라이버를 통해 모터를 제어합니다. 
상호 TLS(mTLS) 인증을 통해 보안을 강화하고, 구조화된 JSON 메시지 형식을 사용합니다.

## 주요 기능

- 상호 TLS(mTLS) 인증을 통한 보안 강화
- 인증서 기반 디바이스 식별
- 구조화된 JSON 메시지 형식
- 표준화된 토픽 구조
- 모터 제어 기능 유지
- 안전한 종료 처리 (Ctrl+C)
- 실시간 상태 표시

## 🔧 시스템 요구사항

- 라즈베리파이 (Linux)
- L298N 모터 드라이버 커널 모듈
- 필요한 패키지:
  - libmosquitto-dev
  - libssl-dev

## 💾 빌드 및 설치

### 빌드 방법

```bash
mkdir build
cd build
cmake ..
make
```

### 빌드 결과
```
build/
├── tls_mqtt_conveyor      # 실행 파일
└── certs/                 # 인증서 디렉토리
    ├── ca.crt
    ├── conveyor_02.crt
    └── conveyor_02.key
```

## 🚀 실행 방법

```bash
./tls_mqtt_conveyor
```

**⚠️ 주의**: 디바이스 파일 접근을 위해 sudo 권한이 필요할 수 있습니다.

## 토픽 구조

- 구독: `factory/{device_id}/cmd`
- 발행: `factory/{device_id}/status`

## 명령어

프로그램은 지정된 토픽에서 다음 페이로드를 인식합니다:

| 페이로드 | 동작 | 설명 |
|---------|------|------|
| `on` | 모터 켜짐 | 모터A 정방향 99% 속도 |
| `off` | 모터 끄짐 | 모든 모터 정지 |

* 대소문자 구분 없음 (ON, Off, oN 등 모두 인식)

### MQTT 메시지 전송 예시

```bash
# 모터 켜기
mosquitto_pub -h mqtt.example.com -p 8883 --cafile ca.crt --cert conveyor_02.crt --key conveyor_02.key -t "factory/conveyor_02/cmd" -m "on"

# 모터 끄기
mosquitto_pub -h mqtt.example.com -p 8883 --cafile ca.crt --cert conveyor_02.crt --key conveyor_02.key -t "factory/conveyor_02/cmd" -m "off"
```

## 인증서 설정

인증서 파일은 `certs` 디렉토리에 위치해야 합니다:
- `ca.crt`: CA 인증서
- `conveyor_02.crt`: 클라이언트 인증서
- `conveyor_02.key`: 클라이언트 개인키

## 🔧 문제해결

### SSL 연결 문제

#### 1. SSL 연결 실패
```bash
# 네트워크 연결 확인
ping mqtt.example.com

# SSL 포트 확인
nmap -p 8883 mqtt.example.com

# 방화벽 확인
sudo ufw status
```

#### 2. 인증서 파일 없음
```bash
# 오류 메시지
CA certificate file not found: ./certs/ca.crt

# 해결방법
ls -la certs/ca.crt  # 파일 존재 확인
cp /path/to/correct/ca.crt ./certs/  # 올바른 위치에서 복사
```

### 모터 제어 문제

#### 1. 디바이스 파일 접근 오류
```bash
# 오류 메시지
디바이스가 열려있지 않습니다.

# 해결방법
ls -l /dev/l298n_motor  # 디바이스 파일 확인
sudo chmod 666 /dev/l298n_motor  # 권한 설정
```

#### 2. 권한 문제
```bash
# sudo 권한으로 실행
sudo ./tls_mqtt_conveyor

# 사용자를 적절한 그룹에 추가
sudo usermod -a -G gpio $USER
```

### MQTT 메시지 문제

#### 1. 메시지가 도착하지 않음
```bash
# 구독 테스트
mosquitto_sub -h mqtt.example.com -p 8883 --cafile ca.crt --cert conveyor_02.crt --key conveyor_02.key -t "factory/conveyor_02/cmd"

# 발행 테스트
mosquitto_pub -h mqtt.example.com -p 8883 --cafile ca.crt --cert conveyor_02.crt --key conveyor_02.key -t "factory/conveyor_02/cmd" -m "test"
```

## 💡 사용 팁

### 1. 자동 시작 설정
시스템 부팅 시 자동으로 실행하려면:
```bash
# systemd 서비스 파일 생성
sudo nano /etc/systemd/system/tls-mqtt-conveyor.service
```

```ini
[Unit]
Description=TLS MQTT Conveyor Control
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=/path/to/tls_mqtt_conveyor/build
ExecStart=/path/to/tls_mqtt_conveyor/build/tls_mqtt_conveyor
Restart=always

[Install]
WantedBy=multi-user.target
```

```bash
# 서비스 활성화
sudo systemctl enable tls-mqtt-conveyor.service
sudo systemctl start tls-mqtt-conveyor.service
```

### 2. 로그 파일 생성
실행 로그를 파일로 저장:
```bash
sudo ./tls_mqtt_conveyor > /var/log/tls-mqtt-conveyor.log 2>&1 &
```
