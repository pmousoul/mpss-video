// Server client service include file


#ifndef SERVER_CLIENT_SERVICE_H
#define SERVER_CLIENT_SERVICE_H


/* Include files:
*****************/
// Common:
#include "common.h"
// OS Specific - part of the operating system:
#include <unistd.h> // Used for introducing delays
// Application Specific:
#include "server_client_service_message.h" // Server client service header
#include "server_video_pipeline.h" // Server video pipeline header
#include "video_pipeline_message.h" // Message sent to the video pipeline functions



/* Function declarations:
*************************/
void close_tcp_socket_and_thread(TCPSocket *sock, uint16_t client_id);
void video_port_request_service(TCPSocket *sock, uint16_t *cid, uint16_t server_port, uint16_t *vport);

#endif
