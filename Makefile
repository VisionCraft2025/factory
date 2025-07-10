# L298N 모터 드라이버 커널 모듈 Makefile

# 모듈 이름
obj-m += l298n_motor_driver.o

# 커널 빌드 디렉토리
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build

# 현재 디렉토리
PWD := $(shell pwd)

# 컴파일 플래그
ccflags-y := -Wall -Wextra

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f *.symvers *.order

install: all
	sudo insmod l298n_motor_driver.ko
	@echo "모듈이 로드되었습니다."
	@echo "디바이스 파일: /dev/l298n_motor"
	@dmesg | tail -5

uninstall:
	sudo rmmod l298n_motor_driver
	@echo "모듈이 언로드되었습니다."

status:
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

help:
	@echo "L298N 모터 드라이버 빌드 시스템"
	@echo ""
	@echo "사용 가능한 타겟:"
	@echo "  all       - 모듈 컴파일"
	@echo "  clean     - 빌드 파일 정리"
	@echo "  install   - 모듈 컴파일 및 로드"
	@echo "  uninstall - 모듈 언로드"
	@echo "  status    - 모듈 및 디바이스 상태 확인"
	@echo "  test      - 간단한 모터 테스트 실행"
	@echo "  help      - 이 도움말 출력"

.PHONY: all clean install uninstall status test help