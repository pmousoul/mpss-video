// Helper function definitions


#include "client.h"


/* Function definitions:
************************/

/*
 * Function definition used to sent video port request to server and
 * receive an answer - through TCP:
 */
uint16_t video_port_request(const char *server_ip, uint16_t server_port, uint8_t client_id) {

	// Message used for client-server communication
	uint16_t message;


	// Loop until a video port is received successfully
	while ( 1 ) {

		// Connect to the server
		TCPSocket sock(server_ip, server_port);


		// Convert client's ID to network byte order and send it to the server
		message = client_id;
		DEBUG("Sending ID: " << message << " to Server");
		message = htons(message);
		if ( sock.send(&message, sizeof(message)) == sizeof(message) ) {
			DEBUG("Send successful");
		} else {
			sock.close();
			DEBUG("Client's ID could not be sent properly" << std::endl <<
				"Client's TCP socket closed - retrying");
			continue;
		}


		// Read the server's response, convert it to local byte order and print it
		if ( int rc=sock.recvFully(&message, sizeof(message)) == sizeof(message) ) {
			message = ntohs(message);
			DEBUG("Server's response: " << message);
		} else {
			DEBUG("Server's response could not received properly");
			if ( rc == 0 )
				DEBUG("The connection was closed by the other end");
			else
				DEBUG("Wrong number of bytes received");
			sock.close();
			DEBUG("Client's TCP socket closed - retrying");
			continue;
		}


		// Close the TCP socket
		sock.close();
		DEBUG("Client's TCP socket closed");


		// Check if a valid port number has been received
		if ( message == ( server_port + client_id ) ) {
			 break;
		} else {
			 DEBUG("Client did not receive the video port successfully");
			 continue;
		}


	}

	return message;


}
