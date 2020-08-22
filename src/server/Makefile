# Compiler to be used:
CC=g++

# Options passed to the compiler:
CXXFLAGS = \
-D USEDEBUG \
-I../include/ \
`pkg-config --cflags gstreamer-1.0`

LDFLAGS = `pkg-config --libs gstreamer-1.0` -lpthread

COMMON_SOURCES = ../common/Socket.cpp ../common/Thread.cpp
SOURCES=$(wildcard *.cpp)

COMMON_OBJECTS = $(COMMON_SOURCES:.cpp=.o)
OBJECTS = $(SOURCES:.cpp=.o)

Server: $(COMMON_OBJECTS) $(OBJECTS)
	$(CC) $(COMMON_OBJECTS) $(OBJECTS) $(LDFLAGS) -o $@

Server_DEBUG_FPS: CXXFLAGS += -D DEBUG_FPS
Server_DEBUG_FPS: $(COMMON_OBJECTS) $(OBJECTS)
	$(CC) $(COMMON_OBJECTS) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CC) -c $(CXXFLAGS) $< -o $@

clean:
	rm $(OBJECTS) $(COMMON_OBJECTS) Server*
