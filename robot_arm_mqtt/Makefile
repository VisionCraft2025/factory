# Robot Arm Control System Makefile

# 유저 프로그램
TARGET1 = robot_arm_user
SRC1 = robot_arm_user.cpp

# MQTT 프로그램
TARGET2 = robot_arm_mqtt
SRC2 = robot_arm_mqtt.cpp

# MQTT TLS 프로그램
TARGET3 = robot_arm_mqtt_tls
SRC3 = robot_arm_mqtt_tls.cpp

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

# Qt6 MQTT 라이브러리 경로 (로컬 빌드)
QT_MQTT_DIR = $(HOME)/dev/cpp_libs/qtmqtt/install/usr
QT_INCLUDE_DIR = $(QT_MQTT_DIR)/include/aarch64-linux-gnu/qt6
QT_LIBDIR = $(QT_MQTT_DIR)/lib/aarch64-linux-gnu

# 환경변수 설정
export PKG_CONFIG_PATH := $(QT_LIBDIR)/pkgconfig:$(PKG_CONFIG_PATH)
export LD_LIBRARY_PATH := $(QT_LIBDIR):$(LD_LIBRARY_PATH)

QT_FLAGS = -I$(QT_INCLUDE_DIR) -L$(QT_LIBDIR) `pkg-config --cflags --libs Qt6Mqtt Qt6Core`

all: $(TARGET1) $(TARGET2) $(TARGET3) $(DRIVER_OBJ)

$(TARGET1): $(SRC1)
	$(CXX) $(CXXFLAGS) -o $(TARGET1) $(SRC1)

$(TARGET2): $(SRC2)
	$(CXX) $(CXXFLAGS) -o $(TARGET2) $(SRC2) $(QT_FLAGS)

$(TARGET3): $(SRC3)
	$(CXX) $(CXXFLAGS) -o $(TARGET3) $(SRC3) -lmosquitto -lssl -lcrypto

$(DRIVER_OBJ):
	@echo "obj-m := robot_arm_driver.o" > Kbuild
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# 테스트 프로그램 (선택사항)
$(TEST_TARGET): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_SRC) $(LIBS)

clean:
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) $(TEST_TARGET) Kbuild
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# 개발용 타겟들
test: $(TEST_TARGET)

.PHONY: all clean test