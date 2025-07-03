# Robot Arm MQTT Control System

4축 서보모터 기반 로봇팔을 MQTT 프로토콜로 원격 제어하는 시스템입니다.

---

## 하드웨어 구성

| 서보모터 | GPIO 핀 | 기능 |
|---------|---------|------|
| servo0 | 22 | 하단 모터 |
| servo1 | 24 | 중단 모터 |
| servo2 | 25 | 상단 모터 |
| servo3 | 8 | 엔드 모터(그리퍼) |

---

## 설치 가이드

### Step 1: 시스템 준비
```bash
sudo apt update && sudo apt upgrade -y
sudo apt install build-essential linux-headers-$(uname -r) mosquitto-clients -y
```

### Step 2: Qt6 MQTT 설정
기존 tls_mqtt 라이브러리 설치 경로를 사용합니다.
```
~/cv_practice/project/pproject/lab/robot_arm_pracice/robot_arm_mqtt/tls_mqtt/
```

### Step 3: 프로젝트 실행
```bash
cd robot_arm_mqtt
chmod +x setup_robot_arm.sh
sudo ./setup_robot_arm.sh
```

---

## 제어 모드

###  Manual Mode
```
실행 후 "1" 선택
> auto_on      ← 자동 시퀀스 실행
> servo0 90    ← 베이스를 90도로 회전
> init         ← 초기 위치로 복귀
> exit         ← 프로그램 종료
```

###  MQTT Mode (원격 제어)
```
실행 후 "2" 선택
토픽: robot_arm/cmd
브로커: mqtt.kwon.pics:1883
```

---

## MQTT 명령어 레퍼런스

### 기본 제어
```bash
# 시스템 제어
mosquitto_pub -h mqtt.kwon.pics -t robot_arm/cmd -m "init"      # 초기화
mosquitto_pub -h mqtt.kwon.pics -t robot_arm/cmd -m "auto_on"   # 자동 시작
mosquitto_pub -h mqtt.kwon.pics -t robot_arm/cmd -m "auto_off"  # 자동 중지
```

### 위치 제어
```bash
# 수동 제어 (0-250도)
mosquitto_pub -h mqtt.kwon.pics -t robot_arm/cmd -m "servo0 45"
mosquitto_pub -h mqtt.kwon.pics -t robot_arm/cmd -m "servo1 120"
mosquitto_pub -h mqtt.kwon.pics -t robot_arm/cmd -m "servo2 180"
mosquitto_pub -h mqtt.kwon.pics -t robot_arm/cmd -m "servo3 90"
```

---

## 자동 모드 동작

| 단계 | 동작 설명 | 소요 시간 |
|------|----------|----------|
| 0 | 픽업 위치로 이동 및 물체 집기 | ~3초 |
| 1 | 물체 들어올리기 | ~2초 |
| 2 | 배치 위치로 이동 | ~3초 |
| 3 | 물체 배치 후 초기 위치 복귀 | ~4초 |

---
