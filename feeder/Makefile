# test용 feeder.cpp 
TARGET1 = feeder
SRC1 = feeder.cpp

#유저 프로그램 feeder_user.cpp 
TARGET2 = feeder_user
SRC2 = feeder_user.cpp

CXX = g++
CXXFLAGS = -Wall
LIBS = -lwiringPi

#커널 모듈
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
DRIVER_OBJ = feeder_driver.ko

all: $(TARGET1) $(TARGET2) $(DRIVER_OBJ)

$(TARGET1): $(SRC1)
	$(CXX) $(CXXFLAGS) -o $(TARGET1) $(SRC1) $(LIBS)

$(TARGET2): $(SRC2)
	$(CXX) $(CXXFLAGS) -o $(TARGET2) $(SRC2)

$(DRIVER_OBJ):
	$(MAKE) -C $(KDIR) M=$(PWD) modules



clean:
	rm -f $(TARGET1) $(TARGET2)
	$(MAKE) -C $(KDIR) M=$(PWD) clean
