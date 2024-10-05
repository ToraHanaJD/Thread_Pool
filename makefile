CXX = g++

CXXFLAGS = -Wall -g -std=c++20 -pthread -fcoroutines

SRCS = $(wildcard *.cpp)
HDRS = $(wildcard *.hpp)

OBJS = $(SRCS:.cpp=.o)

TARGET = main

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)