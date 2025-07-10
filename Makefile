# obj-m += conveyor_driver.o

# USER_APP = conveyor_user
# MQTT_APP = conveyor_mqtt

# # Qt6 MQTT 경로 설정
# QT6MQTT_BASE = /home/veda/dev/cpp_libs/qtmqtt/install
# QT6MQTT_PKGCONFIG = $(QT6MQTT_BASE)/usr/lib/aarch64-linux-gnu/pkgconfig
# QT6MQTT_LIB = $(QT6MQTT_BASE)/usr/lib/aarch64-linux-gnu

# all: module user_app mqtt_app

# module:
# 	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

# user_app:
# 	g++ -o $(USER_APP) $(USER_APP).cpp

# mqtt_app:
# 	g++ -I/usr/include/aarch64-linux-gnu/qt6 \
# 	    -I/usr/include/aarch64-linux-gnu/qt6/QtCore \
# 	    -I/usr/include/aarch64-linux-gnu/qt6/QtNetwork \
# 	    -I/home/veda/dev/cpp_libs/qtmqtt/install/usr/include/aarch64-linux-gnu/qt6 \
# 	    -I/home/veda/dev/cpp_libs/qtmqtt/install/usr/include/aarch64-linux-gnu/qt6/QtMqtt \
# 	    -L/home/veda/dev/cpp_libs/qtmqtt/install/usr/lib/aarch64-linux-gnu \
# 	    -o $(MQTT_APP) $(MQTT_APP).cpp \
# 	    -lQt6Mqtt -lQt6Core -lQt6Network

		
# clean:
# 	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
# 	rm -f $(USER_APP) $(MQTT_APP)

# install: all
# 	sudo insmod conveyor_driver.ko
# 	sleep 1
# 	$(eval MAJOR := $(shell cat /proc/devices | grep conveyor_mqtt | awk '{print $$1}'))
# 	sudo mknod /dev/conveyor_mqtt c $(MAJOR) 0 2>/dev/null || true
# 	sudo chmod 666 /dev/conveyor_mqtt

# uninstall:
# 	sudo rmmod conveyor_driver 2>/dev/null || true
# 	sudo rm -f /dev/conveyor_mqtt

obj-m += conveyor_driver.o

USER_APP = conveyor_user
MQTT_APP = conveyor_mqtt

# Qt6 MQTT 경로 설정
QT6MQTT_BASE = /home/veda/dev/cpp_libs/qtmqtt/install
QT6MQTT_INCLUDE = $(QT6MQTT_BASE)/usr/include/aarch64-linux-gnu/qt6
QT6MQTT_LIB = $(QT6MQTT_BASE)/usr/lib/aarch64-linux-gnu

# 시스템 Qt6 경로
QT6_SYSTEM_INCLUDE = /usr/include/aarch64-linux-gnu/qt6
QT6_SYSTEM_LIB = /usr/lib/aarch64-linux-gnu

all: module user_app mqtt_app

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

user_app:
	g++ -o $(USER_APP) $(USER_APP).cpp

mqtt_app:
	@echo "컴파일 중: $(MQTT_APP)"
	g++ -std=c++17 \
	    -I$(QT6_SYSTEM_INCLUDE) \
	    -I$(QT6_SYSTEM_INCLUDE)/QtCore \
	    -I$(QT6_SYSTEM_INCLUDE)/QtNetwork \
	    -I$(QT6MQTT_INCLUDE) \
	    -I$(QT6MQTT_INCLUDE)/QtMqtt \
	    -L$(QT6_SYSTEM_LIB) \
	    -L$(QT6MQTT_LIB) \
	    -Wl,-rpath,$(QT6MQTT_LIB) \
	    -o $(MQTT_APP) $(MQTT_APP).cpp \
	    -lQt6Mqtt -lQt6Core -lQt6Network
	@echo "컴파일 완료: $(MQTT_APP)"

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f $(USER_APP) $(MQTT_APP)

install: all
	@echo "=== 모듈 설치 중 ==="
	sudo insmod conveyor_driver.ko
	@echo "모듈 로드 완료, Major 번호 확인 중..."
	sleep 2
	@echo "=== 커널 로그 확인 ==="
	dmesg | tail -5
	@echo "=== Major 번호 감지 ==="
	@$(eval MAJOR_NUM := $(shell cat /proc/devices | grep conveyor_mqtt | awk '{print $$1}'))
	@echo "감지된 Major 번호: '$(MAJOR_NUM)'"
	@if [ -z "$(MAJOR_NUM)" ]; then \
		echo "Major 번호를 찾을 수 없습니다. /proc/devices 확인:"; \
		cat /proc/devices | grep conveyor; \
		exit 1; \
	fi
	@echo "디바이스 파일 생성 중... (Major: $(MAJOR_NUM))"
	sudo mknod /dev/conveyor_mqtt c $(MAJOR_NUM) 0 2>/dev/null || true
	sudo chmod 666 /dev/conveyor_mqtt
	@if [ -e /dev/conveyor_mqtt ]; then \
		echo "✓ 디바이스 파일 생성 성공!"; \
		ls -la /dev/conveyor_mqtt; \
	else \
		echo "✗ 디바이스 파일 생성 실패!"; \
		exit 1; \
	fi
	@echo "=== 설치 완료! ==="

uninstall:
	@echo "모듈 제거 중..."
	sudo rmmod conveyor_driver 2>/dev/null || true
	sudo rm -f /dev/conveyor_mqtt
	@echo "제거 완료!"

# Major 번호 수동 생성 (백업 방법)
create_device:
	@echo "=== 수동 디바이스 파일 생성 ==="
	@echo "/proc/devices 내용:"
	@cat /proc/devices | grep -E "(conveyor|Character)"
	@echo "Major 번호를 입력하세요:"
	@read MAJOR && sudo mknod /dev/conveyor_mqtt c $$MAJOR 0
	@sudo chmod 666 /dev/conveyor_mqtt
	@ls -la /dev/conveyor_mqtt

test: install
	@echo "=== 모듈 테스트 ==="
	lsmod | grep conveyor
	ls -la /dev/conveyor_mqtt
	@echo "=== 직접 테스트 ==="
	echo "on" > /dev/conveyor_mqtt
	sleep 1
	cat /dev/conveyor_mqtt
	echo "off" > /dev/conveyor_mqtt

mqtt: mqtt_app
	@echo "=== MQTT 클라이언트 실행 ==="
	@echo "환경 변수 설정 중..."
	export LD_LIBRARY_PATH=$(QT6MQTT_LIB):$$LD_LIBRARY_PATH && ./$(MQTT_APP)

debug:
	@echo "=== 디버그 정보 ==="
	@echo "현재 로드된 모듈:"
	lsmod | grep conveyor
	@echo "디바이스 파일:"
	ls -la /dev/conveyor_mqtt 2>/dev/null || echo "디바이스 파일 없음"
	@echo "/proc/devices에서 conveyor 검색:"
	cat /proc/devices | grep conveyor || echo "없음"
	@echo "최근 커널 로그:"
	dmesg | tail -10

help:
	@echo "사용 가능한 명령어:"
	@echo "  make all         - 모든 빌드"
	@echo "  make install     - 모듈 설치 및 디바이스 파일 생성"
	@echo "  make uninstall   - 모듈 제거"
	@echo "  make test        - 설치 후 기본 테스트"
	@echo "  make mqtt        - MQTT 클라이언트 실행"
	@echo "  make debug       - 현재 상태 확인"
	@echo "  make create_device - 수동 디바이스 파일 생성"
	@echo "  make clean       - 빌드 파일 정리"

.PHONY: all module user_app mqtt_app clean install uninstall test mqtt help debug create_device