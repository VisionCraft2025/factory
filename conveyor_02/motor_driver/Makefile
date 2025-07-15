# L298N 모터 드라이버 커널 모듈 Makefile

# 모듈 이름
obj-m += l298n_motor_driver.o

# 커널 빌드 디렉토리
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build

# 현재 디렉토리와 빌드 디렉토리
PWD := $(shell pwd)
BUILD_DIR := $(PWD)/build

# 컴파일 플래그
ccflags-y := -Wall -Wextra

# 빌드 디렉토리 생성
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

all: $(BUILD_DIR)
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
	@mv *.ko $(BUILD_DIR)/ 2>/dev/null || true
	@mv *.o $(BUILD_DIR)/ 2>/dev/null || true
	@mv *.mod.c $(BUILD_DIR)/ 2>/dev/null || true
	@mv .*.cmd $(BUILD_DIR)/ 2>/dev/null || true
	@mv modules.order $(BUILD_DIR)/ 2>/dev/null || true
	@mv Module.symvers $(BUILD_DIR)/ 2>/dev/null || true
	@echo "빌드 완료: $(BUILD_DIR)/l298n_motor_driver.ko"

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -rf $(BUILD_DIR)
	rm -f *.symvers *.order
	@echo "빌드 파일 정리 완료"

install: all
	@echo "기존 모듈 확인 및 언로드..."
	@sudo rmmod l298n_motor_driver 2>/dev/null || true
	@echo "새 모듈 로드 중..."
	sudo insmod $(BUILD_DIR)/l298n_motor_driver.ko
	@echo "모듈이 로드되었습니다."
	@echo "디바이스 파일: /dev/l298n_motor"
	@sudo chmod 666 /dev/l298n_motor 2>/dev/null || true
	@dmesg | tail -5

uninstall:
	@echo "모듈 언로드 중..."
	@sudo rmmod l298n_motor_driver 2>/dev/null || echo "모듈이 로드되지 않음"
	@echo "모듈이 언로드되었습니다."

reload: uninstall install
	@echo "모듈 재로드 완료!"

force-install: all
	@echo "강제 모듈 재설치..."
	@sudo rmmod l298n_motor_driver 2>/dev/null || true
	sudo insmod $(BUILD_DIR)/l298n_motor_driver.ko
	@sudo chmod 666 /dev/l298n_motor 2>/dev/null || true
	@echo "강제 설치 완료!"

status:
	@echo "=== 빌드 상태 ==="
	@if [ -f $(BUILD_DIR)/l298n_motor_driver.ko ]; then \
		echo "빌드 완료: $(BUILD_DIR)/l298n_motor_driver.ko"; \
	else \
		echo "빌드되지 않음 (make를 실행하세요)"; \
	fi
	@echo ""
	@echo "=== 모듈 상태 ==="
	@lsmod | grep l298n || echo "모듈이 로드되지 않음"
	@echo ""
	@echo "=== 디바이스 파일 ==="
	@ls -l /dev/l298n_motor 2>/dev/null || echo "디바이스 파일이 없음"
	@echo ""
	@echo "=== 최근 커널 메시지 ==="
	@dmesg | grep "L298N" | tail -5 || echo "관련 메시지 없음"

test:
	@echo "=== L298N 모터 드라이버 테스트 ==="
	@if [ ! -c /dev/l298n_motor ]; then \
		echo "오류: 디바이스 파일이 없습니다. 먼저 'make install'을 실행하세요."; \
		exit 1; \
	fi
	@echo "모터A 정방향 테스트..."
	@echo "A 1 100" | sudo tee /dev/l298n_motor > /dev/null
	@sleep 2
	@echo "모터A 역방향 테스트..."
	@echo "A -1 80" | sudo tee /dev/l298n_motor > /dev/null
	@sleep 2
	@echo "모터B 정방향 테스트..."
	@echo "B 1 60" | sudo tee /dev/l298n_motor > /dev/null
	@sleep 2
	@echo "모든 모터 정지..."
	@echo "S 0 0" | sudo tee /dev/l298n_motor > /dev/null
	@echo "테스트 완료!"

debug:
	@echo "=== 디버그 정보 ==="
	@echo "커널 버전: $(shell uname -r)"
	@echo "빌드 디렉토리: $(BUILD_DIR)"
	@echo "커널 빌드 디렉토리: $(KERNEL_DIR)"
	@echo ""
	@echo "=== GPIO 상태 ==="
	@sudo cat /sys/kernel/debug/gpio 2>/dev/null | grep "GPIO[0-9]*:" | head -10 || echo "GPIO 정보 접근 불가"
	@echo ""
	@echo "=== 모듈 정보 ==="
	@if [ -f $(BUILD_DIR)/l298n_motor_driver.ko ]; then \
		modinfo $(BUILD_DIR)/l298n_motor_driver.ko; \
	else \
		echo "모듈 파일이 없습니다."; \
	fi

help:
	@echo "L298N 모터 드라이버 빌드 시스템"
	@echo ""
	@echo "사용 가능한 타겟:"
	@echo "  all          - 모듈 컴파일 (build/ 디렉토리에 저장)"
	@echo "  clean        - 빌드 파일 정리 (build/ 디렉토리 삭제)"
	@echo "  install      - 모듈 컴파일 및 로드 (기존 모듈 자동 언로드)"
	@echo "  uninstall    - 모듈 언로드"
	@echo "  reload       - 모듈 언로드 후 재설치"
	@echo "  force-install- 강제 모듈 재설치"
	@echo "  status       - 빌드 및 모듈 상태 확인"
	@echo "  test         - 간단한 모터 테스트 실행"
	@echo "  debug        - 상세 디버그 정보 출력"
	@echo "  help         - 이 도움말 출력"
	@echo ""
	@echo "빌드 결과물:"
	@echo "  $(BUILD_DIR)/l298n_motor_driver.ko - 커널 모듈"
	@echo "  $(BUILD_DIR)/*.o                   - 오브젝트 파일"

.PHONY: all clean install uninstall reload force-install status test debug help