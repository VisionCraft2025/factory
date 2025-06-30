# factory

## Feeder
### 실행 방법

### 1. 의존 패키지 설치 (최초 1회)
```bash
sudo apt update
sudo apt install -y build-essential raspberrypi-kernel-headers wiringpi
```

### 2. 프로젝트 빌드
```bash
make
```
### 3. 커널 모듈 로딩 및 실행
```bash
sudo ./setup_feeder.sh
```
/dev/feeder 생성

모듈이 로드되며 major 번호 자동 인식

feeder_user 실행 후 명령어 입력 대기

> on        # 정방향 회전

> reverse   # 역방향 회전

> off       # 회전 정지

> exit      # 종료
