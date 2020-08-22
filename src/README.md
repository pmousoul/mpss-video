Introduction
------------
This folder contains the C/C++ code for the client-server video application of the Mobile Personal Supervision System (MPSS).


How to compile Client and Server programs for testing:
------------------------------------------------------
To build the Client part (for plain webcam):
cd client
make clean
make Client_H264C

To build the Client part (for H264 capable webcam, e.g. Logitech C920 camera module):
cd client
make clean
make Client_H264N_C920

To build the Client part (for the Raspberry PI camera module):
cd client
make clean
make Client_H264N_C920

To build the Server part:
cd server
make clean
make Server

To build the Server part with DEBUG_FPS support:
cd server
make clean
make Server_DEBUG_FPS


How to run video_client_server:
-------------------------------
First start the Server part:
./Server 127.0.0.1 3000

Then start the client part:
./Client_H264C 127.0.0.1 3000 1 /dev/video0 640 480 30

# Start the vlc http mpjpeg stream server:
cvlc --network-caching=50 --loop udp://@:3021 --sout '#standard{access=http{mime=multipart/x-mixed-replace;boundary=7b3cc56e5f51db803f790dad720ed50a},mux=mpjpeg,dst=127.0.0.1:8182/mpss.mpjpeg}'

# View the mpjpeg stream:
cvlc --network-caching=50 --no-autoscale --loop http://127.0.0.1:8182/mpss.mpjpeg
