# factory

## Feeder

feeder 핀 번호: 5, 6, 13, 19

### 실행 방법

### 1. 의존 패키지 설치 (최초 1회)
```bash
sudo apt update
sudo apt install -y build-essential raspberrypi-kernel-headers wiringpi mosquitto-clients
```

### 2. MQTT 모듈 설치 (최초 1회만)
Qt6용 MQTT 라이브러리는 기본 설치되어 있지 않으므로 직접 빌드합니다.
병수님이 작성하신 tls_mqtt 깃 레포 부분 따라하기.

### 3. 빌드 / 커널 모듈 로딩 및 실행
```bash
sudo ./setup_feeder.sh
```
/dev/feeder 생성

모듈이 로드되며 major 번호 자동 인식

make로 유저 프로그램 및 MQTT 프로그램, 커널 모듈을 자동 빌드

/dev/feeder 생성 및 major 번호 자동 등록

유저 모드 또는 MQTT 모드 선택 가능

#### 유저모드 
> on        # 정방향 회전

> reverse   # 역방향 회전

> off       # 회전 정지

> error     # 에러모드, 30초간 정상 작동 후 점차 느려짐.(멈추진 않음)

> normal    # 정상속도 복귀

> exit      # 종료

#### MQTT 모드
"feeder/cmd" 토픽을 구독

토픽으로 명령어 전송 시 자동으로 /dev/feeder에 write

```bash
mosquitto_pub -h mqtt.kwon.pics -p 1883 -t feeder/cmd -m "on"
mosquitto_pub -h mqtt.kwon.pics -p 1883 -t feeder/cmd -m "reverse"
mosquitto_pub -h mqtt.kwon.pics -p 1883 -t feeder/cmd -m "off"
mosquitto_pub -h mqtt.kwon.pics -p 1883 -t feeder/cmd -m "error"
mosquitto_pub -h mqtt.kwon.pics -p 1883 -t feeder/cmd -m "normal"
```