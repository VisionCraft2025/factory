# test용 feeder.cpp 
TARGET1 = feeder
SRC1 = feeder.cpp

#유저 프로그램 feeder_user.cpp 
TARGET2 = feeder_user
SRC2 = feeder_user.cpp

#mqtt
TARGET3 = feeder_mqtt 
SRC3 = feeder_mqtt.cpp

CXX = g++
CXXFLAGS = -Wall
LIBS = -lwiringPi

# MQTT 관련 환경변수
PKG_CONFIG ?= pkg-config
PKG_MQTT_FLAGS = $(shell $(PKG_CONFIG) --cflags --libs Qt6Mqtt Qt6Core)


#커널 모듈
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
DRIVER_OBJ = feeder_driver.ko

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
	$(CXX) $(CXXFLAGS) -o $(TARGET1) $(SRC1) $(LIBS)

$(TARGET2): $(SRC2)
	$(CXX) $(CXXFLAGS) -o $(TARGET2) $(SRC2)

$(DRIVER_OBJ):
	$(MAKE) -C $(KDIR) M=$(PWD) modules


$(TARGET3): $(SRC3)
	$(CXX) $(CXXFLAGS) -o $(TARGET3) $(SRC3) $(QT_FLAGS)

clean:
	rm -f $(TARGET1) $(TARGET2) $(TARGET3)
	$(MAKE) -C $(KDIR) M=$(PWD) clean
