obj-m += conveyor_driver.o

# 앱 이름
USER_APP = conveyor_user
MQTT_APP = conveyor_mqtt
MQTT_TLS_APP = conveyor_mqtt_tls

# Qt6 경로
QT6_INC = /usr/include/aarch64-linux-gnu/qt6
QT6_LIB = /usr/lib/aarch64-linux-gnu
QTMQTT_BASE = /home/veda/dev/cpp_libs/qtmqtt/install
QTMQTT_INC = $(QTMQTT_BASE)/usr/include/aarch64-linux-gnu/qt6
QTMQTT_LIB = $(QTMQTT_BASE)/usr/lib/aarch64-linux-gnu

all: module user_app mqtt_app mqtt_tls_app

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

user_app:
	g++ -o $(USER_APP) $(USER_APP).cpp

mqtt_app:
	g++ -std=c++17 -o $(MQTT_APP) $(MQTT_APP).cpp \
		-I$(QT6_INC) -I$(QT6_INC)/QtCore -I$(QT6_INC)/QtNetwork \
		-I$(QTMQTT_INC) -I$(QTMQTT_INC)/QtMqtt \
		-L$(QT6_LIB) -L$(QTMQTT_LIB) -Wl,-rpath,$(QTMQTT_LIB) \
		-lQt6Mqtt -lQt6Core -lQt6Network

mqtt_tls_app:
	g++ -std=c++17 -o $(MQTT_TLS_APP) $(MQTT_TLS_APP).cpp \
        -I$(QT6_INC) -I$(QT6_INC)/QtCore -I$(QT6_INC)/QtNetwork \
        -I$(QTMQTT_INC) -I$(QTMQTT_INC)/QtMqtt \
        -L$(QT6_LIB) -L$(QTMQTT_LIB) -Wl,-rpath,$(QTMQTT_LIB) \
        -lQt6Mqtt -lQt6Core -lQt6Network -lmosquitto -lssl -lcrypto
clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f $(USER_APP) $(MQTT_APP) $(MQTT_TLS_APP)

install: all
	sudo insmod conveyor_driver.ko
	sleep 1
	$(eval MAJOR := $(shell cat /proc/devices | grep conveyor_mqtt | awk '{print $$1}'))
	sudo mknod /dev/conveyor_mqtt c $(MAJOR) 0 2>/dev/null || true
	sudo chmod 666 /dev/conveyor_mqtt

uninstall:
	sudo rmmod conveyor_driver 2>/dev/null || true
	sudo rm -f /dev/conveyor_mqtt

test: install
	lsmod | grep conveyor
	ls -la /dev/conveyor_mqtt
	echo "on" > /dev/conveyor_mqtt && sleep 1 && cat /dev/conveyor_mqtt
	echo "off" > /dev/conveyor_mqtt

.PHONY: all module user_app mqtt_app mqtt_tls_app clean install uninstall test
