# Compiler to be used:
CC=g++

# Options passed to the compiler:
CXXFLAGS = \
-D USEDEBUG \
-D CLIENT \
-I../include/ \
`pkg-config --cflags gstreamer-1.0`

LDFLAGS = `pkg-config --libs gstreamer-1.0` -lpthread

COMMON_SOURCES = ../common/Socket.cpp ../common/Thread.cpp
SOURCES=$(wildcard *.cpp)

COMMON_OBJECTS = $(COMMON_SOURCES:.cpp=.o)
OBJECTS = $(SOURCES:.cpp=.o)

Client_H264C: CXXFLAGS += -D H264C
Client_H264C: $(COMMON_OBJECTS) $(OBJECTS)
	$(CC) $(COMMON_OBJECTS) $(OBJECTS) $(LDFLAGS) -o $@

Client_H264N_C920: CXXFLAGS += -D H264N -D C920
Client_H264N_C920: $(COMMON_OBJECTS) $(OBJECTS)
	$(CC) $(COMMON_OBJECTS) $(OBJECTS) $(LDFLAGS) -o $@

Client_H264N_RPI: CXXFLAGS += -D H264N -D RPI
Client_H264N_RPI: $(COMMON_OBJECTS) $(OBJECTS)
	$(CC) $(COMMON_OBJECTS) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CC) -c $(CXXFLAGS) $< -o $@

clean:
	rm $(OBJECTS) $(COMMON_OBJECTS) Client*
