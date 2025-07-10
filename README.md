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
