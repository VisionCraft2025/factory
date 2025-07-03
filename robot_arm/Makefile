# Robot Arm Control System Makefile

# 유저 프로그램
TARGET = robot_arm_user
SRC = robot_arm_user.cpp

# 기존 테스트 프로그램 (참고용)
TEST_TARGET = robot_arm_test
TEST_SRC = robot_arm.cpp

CXX = g++
CXXFLAGS = -Wall -std=c++11
LIBS = -lwiringPi

# 커널 모듈
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
DRIVER_OBJ = robot_arm_driver.ko

# Kbuild 파일 생성을 위한 설정
KBUILD_FILE = Kbuild

all: $(TARGET) $(DRIVER_OBJ)

# 유저 프로그램 빌드
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

# 테스트 프로그램 빌드 (기존 wiringPi 버전)
$(TEST_TARGET): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_SRC) $(LIBS)

# 커널 모듈 빌드를 위한 Kbuild 파일 생성
$(KBUILD_FILE):
	@echo "obj-m := robot_arm_driver.o" > $(KBUILD_FILE)

# 커널 모듈 빌드
$(DRIVER_OBJ): $(KBUILD_FILE)
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# 설치 (선택사항)
install: all
	@echo "Robot Arm 시스템 설치 완료"
	@echo "실행: sudo ./setup_robot_arm.sh"

# 정리
clean:
	rm -f $(TARGET) $(TEST_TARGET) $(KBUILD_FILE)
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# 시스템 실행
run: all
	sudo ./setup_robot_arm.sh


# 개발용 타겟들
test: $(TEST_TARGET)
	@echo "테스트 프로그램 빌드 완료: ./$(TEST_TARGET)"

help:
	@echo "Robot Arm Control System"
	@echo "========================"
	@echo "make          - 전체 빌드"
	@echo "make clean    - 정리"
	@echo "make run      - 시스템 실행"
	@echo "make test     - 테스트 프로그램 빌드"
	@echo "make install  - 설치"
	@echo "make help     - 이 도움말"

.PHONY: all clean install run test help
