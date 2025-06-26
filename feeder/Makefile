# 피더 
TARGET = feeder
SRC = feeder.cpp
CXX = g++
CXXFLAGS = -Wall
LIBS = -lwiringPi

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)
