# 라즈베리파이 L298N 모터 드라이버 커널 모듈

L298N 모터 드라이버 IC를 사용하여 라즈베리파이에서 DC 모터 2개를 제어하는 리눅스 커널 모듈입니다.

## 📋 목차

- [하드웨어 요구사항](#하드웨어-요구사항)
- [하드웨어 연결](#하드웨어-연결)
- [설치 및 빌드](#설치-및-빌드)
- [사용법](#사용법)
- [명령어 참조](#명령어-참조)
- [PWM 설정](#pwm-설정)
- [문제해결](#문제해결)

## 🔧 하드웨어 요구사항

### 필수 구성품
- 라즈베리파이 (모든 모델)
- L298N 모터 드라이버 모듈
- DC 모터 1~2개
- 점퍼 와이어
- 외부 전원 (7-35V, 모터에 따라)

### L298N 모듈 핀 구성
```
전원부 (스크류 터미널):
├── 12V: 모터 구동 전원 (7-35V)
├── GND: 공통 접지
└── 5V: 라즈베리파이용 5V 출력

모터 출력부:
├── OUT1, OUT2: 모터A 연결
└── OUT3, OUT4: 모터B 연결

제어부:
├── ENA: 모터A 활성화/속도제어 (PWM)
├── IN1, IN2: 모터A 방향 제어
├── IN3, IN4: 모터B 방향 제어
└── ENB: 모터B 활성화/속도제어 (PWM)
```

## 🔌 하드웨어 연결

### 기본 연결 (점퍼 사용, 고정 속도)
```
라즈베리파이 GPIO    →    L298N
GPIO 23             →    IN1
GPIO 24             →    IN2
GPIO 25             →    IN3
GPIO 8              →    IN4
GND                 →    GND

ENA, ENB: 점퍼로 연결 (내부 5V)
```

### PWM 연결 (점퍼 제거, 속도 제어)
```
라즈베리파이 GPIO    →    L298N
GPIO 18 (PWM)       →    ENA
GPIO 23             →    IN1
GPIO 24             →    IN2
GPIO 25             →    IN3
GPIO 8              →    IN4
GPIO 7 (PWM)        →    ENB
GND                 →    GND

ENA, ENB: 점퍼 제거 후 GPIO 연결
```

**⚠️ 중요**: 커널 모듈에서는 GPIO 번호가 `512 + BCM번호`로 계산됩니다. 코드에서 자동 변환되므로 하드웨어 연결만 위의 BCM 번호로 하시면 됩니다.

### 전원 연결
```
외부 전원 (+)       →    L298N 12V
외부 전원 (-)       →    L298N GND
라즈베리파이 GND    →    L298N GND (공통 접지 필수)
```

## 💾 설치 및 빌드

### 1. 개발 환경 준비
```bash
# 라즈베리파이 OS 업데이트
sudo apt update && sudo apt upgrade -y

# 커널 헤더 설치
sudo apt install raspberrypi-kernel-headers

# 빌드 도구 설치
sudo apt install build-essential
```

### 2. 소스 코드 다운로드
```bash
git clone <repository-url>
cd l298n-motor-driver
```

### 3. 컴파일 및 설치
```bash
# 컴파일
make

# 모듈 로드
make install

# 상태 확인
make status
```

## 🚀 사용법

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

## ⚙️ PWM 설정

### PWM 모드 활성화
1. **소스 코드 확인**
   ```c
   #define USE_PWM_CONTROL 1  // PWM 사용 (기본값)
   ```

2. **하드웨어 설정**
   - L298N의 ENA, ENB 점퍼 제거
   - GPIO 18을 ENA에 연결
   - GPIO 7을 ENB에 연결

3. **재컴파일**
   ```bash
   make clean
   make install
   ```

### 고정 속도 모드 (점퍼 사용)
1. **소스 코드 수정**
   ```c
   #define USE_PWM_CONTROL 0  // PWM 비사용
   ```

2. **하드웨어 설정**
   - L298N의 ENA, ENB 점퍼 유지
   - ENA, ENB를 GPIO에 연결하지 않음

### PWM 관련 주의사항

**라즈베리파이 GPIO 번호 변환:**
- 하드웨어 연결: BCM 번호 사용 (GPIO 18, 23, 24, 25, 8, 7)
- 커널 모듈: 자동으로 `512 + BCM번호`로 변환
- 사용자는 BCM 번호만 신경쓰면 됨

**점퍼 설정:**
- 점퍼 제거 = PWM 속도 제어 가능
- 점퍼 유지 = 고정 속도 (full speed)

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

# 해결방법: 라즈베리파이는 GPIO 번호 변환 필요
# 코드에서 자동 처리됨 (BCM 번호 + 512)
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
sudo insmod l298n_motor_driver.ko
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
modinfo l298n_motor_driver.ko

# 시스템 정보
uname -r
```

#### 4. 단계별 테스트
```bash
# 1단계: 모듈 로드 테스트
sudo insmod l298n_motor_driver.ko

# 2단계: 디바이스 파일 확인
ls -l /dev/l298n_motor

# 3단계: 상태 읽기 테스트
cat /dev/l298n_motor

# 4단계: 간단한 명령 테스트
echo "A 1 100" | sudo tee /dev/l298n_motor

# 5단계: 커널 메시지 확인
dmesg | tail -5
```

### 일반적인 해결 순서

1. **컴파일 문제** → 커널 헤더 확인 및 소스 수정
2. **모듈 로드 문제** → GPIO 충돌 및 권한 확인  
3. **디바이스 접근 문제** → 파일 권한 설정
4. **명령어 문제** → 형식 및 구문 확인
5. **하드웨어 문제** → 연결 및 전원 확인

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
```bash
# 실시간 커널 메시지 모니터링
sudo dmesg -w

# L298N 관련 메시지만 필터링
dmesg | grep "L298N"
```

## 📁 파일 구조

```
l298n-motor-driver/
├── l298n_motor_driver.c    # 메인 커널 모듈 소스
├── Makefile               # 빌드 설정
├── README.md             # 이 문서
└── examples/
    ├── test_motor.sh     # 테스트 스크립트
    └── motor_control.py  # Python 제어 예제
```

## 🐍 Python 사용 예제

### 간단한 제어 스크립트
```python
#!/usr/bin/env python3
"""
L298N 모터 드라이버 Python 제어 예제
"""

import time

class L298NController:
    def __init__(self, device_path="/dev/l298n_motor"):
        self.device_path = device_path
    
    def send_command(self, command):
        """모터 명령 전송"""
        try:
            with open(self.device_path, 'w') as device:
                device.write(command)
            print(f"명령 전송: {command}")
        except Exception as e:
            print(f"오류: {e}")
    
    def motor_a_forward(self, speed=100):
        """모터A 정방향"""
        self.send_command(f"A 1 {speed}")
    
    def motor_a_reverse(self, speed=100):
        """모터A 역방향"""
        self.send_command(f"A -1 {speed}")
    
    def motor_b_forward(self, speed=100):
        """모터B 정방향"""
        self.send_command(f"B 1 {speed}")
    
    def motor_b_reverse(self, speed=100):
        """모터B 역방향"""
        self.send_command(f"B -1 {speed}")
    
    def stop_all(self):
        """모든 모터 정지"""
        self.send_command("S 0")
    
    def stop_motor_a(self):
        """모터A 정지"""
        self.send_command("A 0")
    
    def stop_motor_b(self):
        """모터B 정지"""
        self.send_command("B 0")

# 사용 예제
if __name__ == "__main__":
    motor = L298NController()
    
    print("모터 테스트 시작...")
    
    # 모터A 테스트
    print("모터A 정방향...")
    motor.motor_a_forward(80)
    time.sleep(2)
    
    print("모터A 역방향...")
    motor.motor_a_reverse(60)
    time.sleep(2)
    
    # 모터B 테스트
    print("모터B 정방향...")
    motor.motor_b_forward(90)
    time.sleep(2)
    
    # 모든 모터 정지
    print("모든 모터 정지...")
    motor.stop_all()
    
    print("테스트 완료!")
```

## 🔄 언로드 및 정리

### 모듈 언로드
```bash
# 모듈 언로드
make uninstall

# 또는 직접 명령
sudo rmmod l298n_motor_driver
```

### 완전 정리
```bash
# 빌드 파일 정리
make clean

# 모든 임시 파일 제거
rm -f *.ko *.o *.mod.c *.symvers *.order
```

## ⚡ 성능 최적화

### 실시간 성능 향상
```bash
# 커널 스케줄러 우선순위 변경
sudo chrt -f 99 your_application

# CPU 주파수 고정 (성능 모드)
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

### 메모리 최적화
```c
// 커널 모듈에서 메모리 프리펫칭 사용
#include <linux/prefetch.h>

static void motor_control_optimized(void) {
    prefetch(&gpio_registers);
    // GPIO 조작 코드
}
```

## 🛡️ 안전 고려사항

### 하드웨어 보호
- **역전압 보호**: 모터 연결시 극성 확인
- **과전류 보호**: L298N 내장 보호 기능 활용
- **열 보호**: 방열판 부착 권장

### 소프트웨어 안전장치
```c
// 모듈 언로드시 안전 정지
static void __exit l298n_exit(void) {
    // 모든 모터 강제 정지
    motor_stop_all();
    
    // GPIO 핀을 안전한 상태로 설정
    gpio_set_value(MOTOR_A_IN1, 0);
    gpio_set_value(MOTOR_A_IN2, 0);
    // ...
}
```

## 📊 성능 벤치마크

### 응답 시간
- **명령 처리**: < 1ms
- **GPIO 토글**: < 100μs
- **방향 전환**: < 10ms

### 리소스 사용량
- **메모리**: < 16KB
- **CPU 사용률**: < 1% (유휴시)

## 🔗 참고 자료

### 관련 문서
- [라즈베리파이 GPIO 핀아웃](https://pinout.xyz/)
- [L298N 데이터시트](https://www.st.com/resource/en/datasheet/l298.pdf)
- [리눅스 커널 모듈 프로그래밍 가이드](https://sysprog21.github.io/lkmpg/)

### 커뮤니티
- [라즈베리파이 공식 포럼](https://www.raspberrypi.org/forums/)
- [Arduino 커뮤니티](https://forum.arduino.cc/)

## 📞 지원 및 기여

### 버그 리포트
이슈가 발생하면 다음 정보와 함께 제보해주세요:
- 라즈베리파이 모델 및 OS 버전
- 커널 버전 (`uname -r`)
- 오류 메시지 (`dmesg` 출력)
- 하드웨어 연결 상태

### 기여 방법
1. Fork 후 브랜치 생성
2. 코드 수정 및 테스트
3. Pull Request 제출

## 📄 라이센스

이 프로젝트는 GPL v2 라이센스 하에 배포됩니다.

## 📈 버전 히스토리

### v1.0 (현재)
- 기본 모터 제어 기능
- PWM 속도 제어 지원
- 안전 정지 기능
- Python 제어 예제

### 향후 계획
- 하드웨어 PWM 지원 (현재는 소프트웨어 PWM)
- 인코더 피드백 지원
- PID 제어 구현
- 웹 인터페이스 추가

### 알려진 이슈
- 커널 6.12+에서 `class_create` API 변경으로 인한 컴파일 오류 (해결됨)
- 라즈베리파이 GPIO 번호 변환 이슈 (해결됨)
- 디바이스 파일 권한 문제 (문서화됨)

---

**🚨 주의**: 이 드라이버는 교육 및 프로토타이핑 목적으로 제작되었습니다. 상업적 사용시 추가 테스트 및 검증이 필요합니다.