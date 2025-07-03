# Robot Arm Control System Makefile

# 유저 프로그램
TARGET1 = robot_arm_user
SRC1 = robot_arm_user.cpp

# MQTT 프로그램
TARGET2 = robot_arm_mqtt
SRC2 = robot_arm_mqtt.cpp

# 기존 테스트 프로그램 (참고용)
TEST_TARGET = robot_arm_test
TEST_SRC = robot_arm.cpp

CXX = g++
CXXFLAGS = -Wall -std=c++17
LIBS = -lwiringPi

# MQTT 관련 환경변수
PKG_CONFIG ?= pkg-config

# 커널 모듈
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
DRIVER_OBJ = robot_arm_driver.ko

# MQTT 헤더 경로 지정 (Robot Arm 경로)
MQTT_BASE_DIR = $(HOME)/cv_practice/project/pproject/lab/robot_arm_pracice/robot_arm_mqtt/tls_mqtt
QT_INCLUDE_DIR = $(MQTT_BASE_DIR)/qt6_local/usr/include/aarch64-linux-gnu/qt6
QT_LIBDIR = $(MQTT_BASE_DIR)/qt6_local/usr/lib/aarch64-linux-gnu

# 환경변수 설정
export PKG_CONFIG_PATH := $(MQTT_BASE_DIR)/qt6_local/usr/lib/aarch64-linux-gnu/pkgconfig:$(PKG_CONFIG_PATH)
export LD_LIBRARY_PATH := $(QT_LIBDIR):$(LD_LIBRARY_PATH)

QT_FLAGS = -I$(QT_INCLUDE_DIR) -L$(QT_LIBDIR) `pkg-config --cflags --libs Qt6Mqtt Qt6Core`

all: $(TARGET1) $(TARGET2) $(DRIVER_OBJ)

$(TARGET1): $(SRC1)
	$(CXX) $(CXXFLAGS) -o $(TARGET1) $(SRC1)

$(TARGET2): $(SRC2)
	$(CXX) $(CXXFLAGS) -o $(TARGET2) $(SRC2) $(QT_FLAGS)

$(DRIVER_OBJ):
	@echo "obj-m := robot_arm_driver.o" > Kbuild
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# 테스트 프로그램 (선택사항)
$(TEST_TARGET): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_SRC) $(LIBS)

clean:
	rm -f $(TARGET1) $(TARGET2) $(TEST_TARGET) Kbuild
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# 개발용 타겟들
test: $(TEST_TARGET)

.PHONY: all clean test