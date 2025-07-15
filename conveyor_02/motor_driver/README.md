# L298N 모터 드라이버 커널 모듈

라즈베리파이에서 L298N 모터 드라이버 IC를 사용하여 DC 모터 2개를 제어하는 리눅스 커널 모듈입니다.

## 📋 목차

- [PWM 설정](#pwm-설정)
- [빌드 및 설치](#빌드-및-설치)
- [사용법](#사용법)
- [명령어 참조](#명령어-참조)
- [문제해결](#문제해결)

## ⚙️ PWM 설정

### PWM 모드 활성화 (권장)
현재 설정은 PWM 속도 제어가 활성화되어 있습니다.

**하드웨어 설정:**
- L298N의 ENA, ENB 점퍼 제거
- GPIO 18을 ENA에 연결
- GPIO 7을 ENB에 연결

### 고정 속도 모드
PWM을 사용하지 않으려면:

1. **소스 코드 수정**
   ```c
   #define USE_PWM_CONTROL 0  // PWM 비사용
   ```

2. **하드웨어 설정**
   - L298N의 ENA, ENB 점퍼 유지
   - ENA, ENB를 GPIO에 연결하지 않음

3. **재컴파일**
   ```bash
   make clean
   make install
   ```

## 💾 빌드 및 설치

### 컴파일 및 설치
```bash
# 컴파일
make

# 모듈 로드
make install

# 상태 확인
make status
```

### 빌드 파일 관리
모든 빌드 결과물은 `build/` 디렉토리에 저장됩니다:
```
build/
├── l298n_motor_driver.ko    # 커널 모듈
├── *.o                      # 오브젝트 파일
└── 기타 빌드 파일들
```

## 🚗 사용법

### 기본 명령어 형식
```bash
echo "<모터> <방향> [속도]" | sudo tee /dev/l298n_motor
```

### 모터 제어 예시
```bash
# 모터A 정방향 회전 (속도 100%)
echo "A 1 100" | sudo tee /dev/l298n_motor

# 모터A 역방향 회전 (속도 80%)
echo "A -1 80" | sudo tee /dev/l298n_motor

# 모터B 정방향 회전 (기본 속도 100%)
echo "B 1" | sudo tee /dev/l298n_motor

# 모터A 정지
echo "A 0" | sudo tee /dev/l298n_motor

# 모든 모터 정지
echo "S 0" | sudo tee /dev/l298n_motor
```

### 자동 테스트 실행
```bash
make test
```

### 상태 확인
```bash
# 드라이버 상태 읽기
cat /dev/l298n_motor

# 커널 메시지 확인
dmesg | grep "L298N"

# 모듈 상태 확인
make status
```

## 📚 명령어 참조

### 모터 제어 명령어

| 모터 | 방향 | 속도 | 설명 |
|------|------|------|------|
| A/a  | 1    | 0-100 | 모터A 정방향 |
| A/a  | -1   | 0-100 | 모터A 역방향 |
| A/a  | 0    | -     | 모터A 정지 (브레이크) |
| B/b  | 1    | 0-100 | 모터B 정방향 |
| B/b  | -1   | 0-100 | 모터B 역방향 |
| B/b  | 0    | -     | 모터B 정지 (브레이크) |
| S/s  | 0    | -     | 모든 모터 정지 |

### 방향 값
- `1`: 정방향 회전
- `-1`: 역방향 회전  
- `0`: 브레이크 (강제 정지)
- `기타`: 프리휠링 (자연 정지)

### 속도 값 (PWM 모드만)
- `0-100`: 속도 퍼센트 (0% = 정지, 100% = 최고속도)
- 생략시 기본값 100%

### Makefile 명령어

| 명령어 | 설명 |
|--------|------|
| `make` | 모듈 컴파일 |
| `make install` | 컴파일 및 모듈 로드 |
| `make uninstall` | 모듈 언로드 |
| `make reload` | 모듈 재로드 |
| `make status` | 모듈 상태 확인 |
| `make test` | 간단한 모터 테스트 |
| `make clean` | 빌드 파일 정리 |
| `make debug` | 디버그 정보 출력 |
| `make help` | 도움말 |

## 🔧 문제해결

### 컴파일 오류

#### 1. class_create 함수 오류 (커널 6.12+)
```bash
# 오류 메시지
error: too many arguments to function 'class_create'

# 해결방법: 소스코드 수정
sed -i 's/class_create(THIS_MODULE, CLASS_NAME)/class_create(CLASS_NAME)/' l298n_motor_driver.c

# 재컴파일
make clean
make install
```

#### 2. 커널 헤더 누락
```bash
# 커널 헤더 재설치
sudo apt update
sudo apt install --reinstall raspberrypi-kernel-headers

# 빌드 파일 정리 후 재컴파일
make clean
make
```

### 모듈 로드 오류

#### 1. GPIO 요청 실패 (Error 517)
```bash
# 원인 확인
dmesg | tail -10

# GPIO 사용 현황 확인
sudo cat /sys/kernel/debug/gpio

# 기존 모듈 제거 후 재시도
sudo rmmod l298n_motor_driver
make install
```

#### 2. 권한 문제
```bash
# 사용자를 gpio 그룹에 추가
sudo usermod -a -G gpio $USER
newgrp gpio

# 디바이스 파일 권한 설정
sudo chmod 666 /dev/l298n_motor
```

#### 3. 기존 모듈 충돌
```bash
# 기존 모듈 확인 및 제거
lsmod | grep -i motor
sudo rmmod l298n_motor_driver

# 재로드
make install
```

### 런타임 오류

#### 1. 디바이스 파일 권한 오류
```bash
# 문제 확인
ls -l /dev/l298n_motor

# 권한 수정
sudo chmod 666 /dev/l298n_motor

# 또는 소유자 변경
sudo chown $USER:$USER /dev/l298n_motor
```

#### 2. 명령어 형식 오류
```bash
# 잘못된 형식
echo "wrong command" | tee /dev/l298n_motor

# 올바른 형식
echo "A 1 100" | tee /dev/l298n_motor
```

### 하드웨어 문제

#### 1. 모터가 동작하지 않는 경우
1. **전원 연결 확인**
   - 외부 전원이 모터 요구 전압과 맞는지 확인
   - GND 공통 연결 필수
   - L298N의 12V 입력 전압 확인

2. **GPIO 연결 확인**
   ```bash
   # GPIO 상태 확인
   sudo cat /sys/kernel/debug/gpio | grep "GPIO1[8,23,24,25]"
   ```

3. **L298N 설정 확인**
   - 점퍼 위치 확인 (PWM 모드 vs 고정 모드)
   - 모터 출력 연결 확인 (OUT1-4)

#### 2. 모터가 한 방향으로만 회전
- IN1, IN2 (또는 IN3, IN4) 연결 상태 확인
- 모터 연결 극성 확인

#### 3. 속도 제어가 안 됨
- ENA/ENB 점퍼가 제거되었는지 확인
- PWM 핀(GPIO 18, 7) 연결 확인
- USE_PWM_CONTROL이 1로 설정되었는지 확인

### 디버깅 방법

#### 1. 실시간 커널 메시지 모니터링
```bash
# 실시간 로그 확인
sudo dmesg -w | grep "L298N"

# 최근 메시지만 확인
dmesg | grep "L298N" | tail -10
```

#### 2. GPIO 상태 확인
```bash
# 모든 GPIO 상태
sudo cat /sys/kernel/debug/gpio

# 특정 GPIO만 확인
sudo cat /sys/kernel/debug/gpio | grep "GPIO[0-9]*"
```

#### 3. 모듈 정보 확인
```bash
# 모듈 로드 상태
lsmod | grep l298n

# 모듈 정보
modinfo build/l298n_motor_driver.ko

# 시스템 정보
uname -r
```

#### 4. 단계별 테스트
```bash
# 1단계: 모듈 로드 테스트
make install

# 2단계: 디바이스 파일 확인
ls -l /dev/l298n_motor

# 3단계: 상태 읽기 테스트
cat /dev/l298n_motor

# 4단계: 간단한 명령 테스트
echo "A 1 100" | sudo tee /dev/l298n_motor

# 5단계: 커널 메시지 확인
dmesg | tail -5
```

### 완전 초기화 방법

문제가 지속될 경우 완전 초기화:

```bash
# 1. 모듈 언로드
sudo rmmod l298n_motor_driver 2>/dev/null || true

# 2. 빌드 파일 정리
make clean

# 3. 재컴파일
make

# 4. 재설치
make install

# 5. 권한 설정
sudo chmod 666 /dev/l298n_motor

# 6. 테스트
echo "A 1 50" | tee /dev/l298n_motor
```

## 💡 주의사항

### GPIO 번호 변환
라즈베리파이에서는 GPIO 번호가 자동으로 변환됩니다:
- 하드웨어 연결: BCM 번호 사용 (GPIO 18, 23, 24, 25, 8, 7)
- 커널 모듈: 자동으로 `512 + BCM번호`로 변환
- 사용자는 BCM 번호만 신경쓰면 됨

### 점퍼 설정
- 점퍼 제거 = PWM 속도 제어 가능
- 점퍼 유지 = 고정 속도 (full speed)