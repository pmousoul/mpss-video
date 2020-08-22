// Server video pipeline thread related structures and functions


#ifndef SERVER_CLIENT_SERVICE_MESSAGE_H
#define SERVER_CLIENT_SERVICE_MESSAGE_H


/* Data type definitions:
**************************/
struct client_service_message {

	TCPSocket *service_sock;
	const char *server_ip;
	uint16_t server_port;


	client_service_message(){} // Default constructor
	client_service_message(
		TCPSocket *sock,
		const char *ip,
		uint16_t port):

		service_sock(sock),
		server_ip(ip),
		server_port(port) {}

};



/* Function declarations:
*************************/
void *client_service(void *thread_arg);

#endif
