# 스마트 팩토리 제어 시스템

라즈베리파이 기반 스마트 팩토리 자동화 시스템 - 컨베이어, 피더, 로봇암을 통합 제어합니다.

## 시스템 구성

```
factory/
├── conveyor01_mqtt/        # 컨베이어01 제어 시스템(재활용 불가한 병 탐지)
├── feeder_mqtt/           # 피더 제어 시스템  
├── robot_arm_mqtt/        # 4축 로봇암 제어 시스템
└── conveyor_02/           # 컨베이어02(직선) 제어 시스템
```

## 하드웨어 연결

### 컨베이어 시스템 (conveyor01_mqtt)
| 컴포넌트 | GPIO 핀 | BCM 번호 | 설명 |
|----------|---------|----------|------|
| 스테퍼 모터 STEP | GPIO 17 | BCM 17 | 스텝 펄스 신호 |
| 스테퍼 모터 DIR | GPIO 27 | BCM 27 | 방향 제어 |
| 스테퍼 모터 ENABLE | GPIO 22 | BCM 22 | 모터 활성화 |
| 서보 모터 PWM | GPIO 5 | BCM 5 | 서보 각도 제어 |

### 피더 시스템 (feeder_mqtt)
| 컴포넌트 | GPIO 핀 | BCM 번호 | 설명 |
|----------|---------|----------|------|
| 모터 제어 1 | GPIO 5 | BCM 5 | 모터 제어 핀 1 |
| 모터 제어 2 | GPIO 6 | BCM 6 | 모터 제어 핀 2 |
| 모터 제어 3 | GPIO 13 | BCM 13 | 모터 제어 핀 3 |
| 모터 제어 4 | GPIO 19 | BCM 19 | 모터 제어 핀 4 |

### 로봇팔 시스템 (robot_arm_mqtt)
| 서보모터 | GPIO 핀 | BCM 번호 | 기능 |
|---------|---------|----------|------|
| servo0 | GPIO 22 | BCM 22 | 하단 모터 (베이스) |
| servo1 | GPIO 24 | BCM 24 | 중단 모터 |
| servo2 | GPIO 25 | BCM 25 | 상단 모터 |
| servo3 | GPIO 8 | BCM 8 | 엔드 모터 (그리퍼) |

### 컨베이어02 시스템 (conveyor_02)
| 컴포넌트 | GPIO 핀 | BCM 번호 | 설명 |
|----------|---------|----------|------|
| 모터A IN1 | GPIO 23 | BCM 23 | 모터A 방향 제어 1 |
| 모터A IN2 | GPIO 24 | BCM 24 | 모터A 방향 제어 2 |
| 모터B IN3 | GPIO 25 | BCM 25 | 모터B 방향 제어 1 |
| 모터B IN4 | GPIO 8 | BCM 8 | 모터B 방향 제어 2 |
| 모터A ENA | GPIO 18 | BCM 18 | 모터A PWM 속도 제어 |
| 모터B ENB | GPIO 7 | BCM 7 | 모터B PWM 속도 제어 |

## 설치 및 실행

### 전체 시스템 설치
```bash
# 의존성 설치
sudo apt update
sudo apt install -y qt6-base-dev qt6-tools-dev build-essential
sudo apt install -y linux-headers-$(uname -r) git

# Qt6 MQTT 라이브러리 설치 (필요시)
mkdir -p ~/dev/cpp_libs
cd ~/dev/cpp_libs
git clone https://github.com/eclipse/qtmqtt.git
cd qtmqtt && mkdir build && cd build
cmake .. && make -j$(nproc)
```

### 1. 컨베이어 시스템 (conveyor01_mqtt)

#### 실행 방법
```bash
cd conveyor01_mqtt
chmod +x setup_conveyor.sh
./setup_conveyor.sh
```

#### 수동 실행
```bash
# 빌드
make all

# 모듈 설치
make install

# 사용자 제어 프로그램
./conveyor_user

# MQTT 제어 프로그램  
./conveyor_mqtt
```

#### MQTT 명령어
```bash
# 컨베이어 시작
mosquitto_pub -h mqtt.kwon.pics -t "conveyor_01/cmd" -m "on"

# 에러 모드 (30초 후 속도 감소)
mosquitto_pub -h mqtt.kwon.pics -t "conveyor_01/cmd" -m "error_mode"

# 컨베이어 정지
mosquitto_pub -h mqtt.kwon.pics -t "conveyor_01/cmd" -m "off"
```

### 2. 피더 시스템 (feeder_mqtt)

#### 실행 방법
```bash
cd feeder_mqtt
sudo ./setup_feeder.sh
```

#### MQTT 명령어
```bash
# 정방향 회전
mosquitto_pub -h mqtt.kwon.pics -p 1883 -t "feeder/cmd" -m "on"

# 역방향 회전
mosquitto_pub -h mqtt.kwon.pics -p 1883 -t "feeder/cmd" -m "reverse"

# 정지
mosquitto_pub -h mqtt.kwon.pics -p 1883 -t "feeder/cmd" -m "off"

# 에러 모드
mosquitto_pub -h mqtt.kwon.pics -p 1883 -t "feeder/cmd" -m "error"

# 정상 속도 복귀
mosquitto_pub -h mqtt.kwon.pics -p 1883 -t "feeder/cmd" -m "normal"

# 암호화 메세지 보내기
mosquitto_pub -h mqtt.kwon.pics -p 8883 --cafile $HOME/certs/ca.crt --cert $HOME/certs/feeder_01.crt --key $HOME/certs/feeder_01.key -t "feeder_01/cmd" -m "on"

```

### 3. 로봇암 시스템 (robot_arm_mqtt)

#### 실행 방법
```bash
cd robot_arm_mqtt
sudo ./setup_robot_arm.sh
```

#### 사용자 제어 모드
```bash
# 실행 후 "1" 선택
> on      # 자동 시퀀스 실행 (재활용 안되는 거 받아서 버리기)
> off     # 자동 종료, 초기값으로 복귀 
> servo0 90    # 베이스를 90도로 회전
> init         # 초기 위치로 복귀
> exit         # 프로그램 종료
```

#### MQTT 명령어
```bash
# 시스템 제어
mosquitto_pub -h mqtt.kwon.pics -t "robot_arm/cmd" -m "init"      # 초기화
mosquitto_pub -h mqtt.kwon.pics -t "robot_arm/cmd" -m "on"   # 자동 시작
mosquitto_pub -h mqtt.kwon.pics -t "robot_arm/cmd" -m "off"  # 자동 중지

# 수동 제어 (0-250도)
mosquitto_pub -h mqtt.kwon.pics -t "robot_arm/cmd" -m "servo0 45"
mosquitto_pub -h mqtt.kwon.pics -t "robot_arm/cmd" -m "servo1 120"
mosquitto_pub -h mqtt.kwon.pics -t "robot_arm/cmd" -m "servo2 180"
mosquitto_pub -h mqtt.kwon.pics -t "robot_arm/cmd" -m "servo3 90"
```

### 4. L298N 모터 드라이버 (conveyor_02)

#### 실행 방법
```bash
cd conveyor_02/motor_driver
make install

# 터미널 제어 프로그램
gcc -o motor_control motor_control.c
./motor_control

# MQTT 제어 프로그램
cd ../mqtt_control
mkdir build && cd build
cmake .. && make
sudo ./mqtt_motor_control
```

#### 직접 제어
```bash
# 모터A 정방향 100% 속도
echo "A 1 100" | sudo tee /dev/l298n_motor

# 모터A 역방향 80% 속도
echo "A -1 80" | sudo tee /dev/l298n_motor

# 모든 모터 정지
echo "S 0 0" | sudo tee /dev/l298n_motor
```

#### MQTT 명령어
```bash
# 모터 켜기
mosquitto_pub -h mqtt.kwon.pics -p 8883 --cafile ca.crt -t "conveyor02/cmd" -m "on"

# 모터 끄기
mosquitto_pub -h mqtt.kwon.pics -p 8883 --cafile ca.crt -t "conveyor02/cmd" -m "off"
```

## 기능 설명

### 컨베이어 시스템
- **자동 컨베이어 제어**: 10칸 자동 순환 (첫 번째 판 6.5cm, 나머지 8.5cm)
- **서보 자동화**: 각 판 도달 시 자동 180도 회전 후 복귀
- **에러 모드**: 30초 정상 작동 후 점진적 속도 감소

### 피더 시스템
- **정/역방향 회전**: 양방향 모터 제어
- **에러 시뮬레이션**: 30초 후 점진적 속도 감소
- **MQTT 원격 제어**: 실시간 상태 모니터링

### 로봇암 시스템
- **4축 서보 제어**: 정밀한 위치 제어 (0-250도)
- **자동 시퀀스**: 물체 감지 → 집기 → 분류 → 배치
- **수동/자동 모드**: 유연한 제어 방식

### L298N 모터 드라이버
- **PWM 속도 제어**: 0-100% 가변 속도
- **듀얼 모터 제어**: 독립적인 2개 모터 제어
- **방향 제어**: 정방향/역방향/브레이크/프리휠링

## MQTT 통신

### 브로커 설정
- **호스트**: mqtt.kwon.pics
- **포트**: 1883 (일반), 8883 (TLS)
- **인증**: TLS 인증서 필요 (일부 시스템)

### 토픽 구조
- `conveyor_01/cmd`: 컨베이어 제어
- `feeder/cmd`: 피더 제어
- `robot_arm/cmd`: 로봇암 제어
- `conveyor02/cmd`: L298N 모터 제어

## 문제 해결

### 권한 오류
```bash
# 디바이스 파일 권한 설정
sudo chmod 666 /dev/conveyor_mqtt
sudo chmod 666 /dev/feeder
sudo chmod 666 /dev/robot_arm
sudo chmod 666 /dev/l298n_motor
```

### 모듈 로드 실패
```bash
# 커널 로그 확인
dmesg | tail -10

# 모듈 상태 확인
lsmod | grep -E "(conveyor|feeder|robot_arm|l298n)"

# 수동 모듈 제거 후 재설치
sudo rmmod [module_name]
make install
```

### MQTT 연결 실패
```bash
# 네트워크 연결 확인
ping mqtt.kwon.pics

# Qt6 MQTT 라이브러리 확인
ldd ./conveyor_mqtt

# 환경 변수 설정
export LD_LIBRARY_PATH=/home/veda/dev/cpp_libs/qtmqtt/install/usr/lib/aarch64-linux-gnu:$LD_LIBRARY_PATH
```

## 사용 팁

### 시스템 시작 순서
1. 각 시스템의 커널 모듈 로드
2. 디바이스 파일 권한 확인
3. MQTT 브로커 연결 확인
4. 제어 프로그램 실행

### 안전 종료
- 모든 프로그램에서 Ctrl+C로 안전 종료
- 종료 시 모터 자동 정지
- 커널 모듈 자동 정리

### 디버깅
```bash
# 실시간 커널 메시지 모니터링
sudo dmesg -w

# GPIO 상태 확인
sudo cat /sys/kernel/debug/gpio

# 디바이스 파일 상태 확인
ls -la /dev/ | grep -E "(conveyor|feeder|robot_arm|l298n)"
```