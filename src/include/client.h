// Client application include file


#ifndef CLIENT_H
#define CLIENT_H


/* Include files:
*****************/
// Common:
#include "common.h" // Client-Server common definitions
// OS Specific - part of the operating system:
#include <unistd.h> // Used for introducing delays
// Application Specific:
#include "client_video_pipeline.h" // Client video pipeline header
#include "video_pipeline_message.h" // Message sent to the client video pipeline function



/* Function declarations:
*************************/
// Used to sent video port request to server and
// receive an answer through TCP:
uint16_t video_port_request(const char *server_ip, uint16_t server_port, uint8_t client_id);

#endif
