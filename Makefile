CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

TARGET := app
SRC := main.cpp

.PHONY: all build run clean

all: build

build: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

run: build
	./$(TARGET)

clean:
	rm -f $(TARGET)
