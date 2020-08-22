// Helper function definitions


#include "server_client_service.h"


/* Function definitions:
************************/

void close_tcp_socket_and_thread(TCPSocket *sock, uint16_t client_id) {

	sock->close();
	DEBUG(client_id << ": " << "TCP socket closed"
		<< std::endl <<client_id << ": " << "Thread closed");
	thread_exit();


}


void video_port_request_service(TCPSocket *sock, uint16_t *cid, uint16_t server_port, uint16_t *vport) {

		uint16_t video_port;
		uint16_t client_id;


		/* Read message from client:
		****************************/
		if ( ( sock->recvFully(&client_id, sizeof(client_id)) ) == ( sizeof(client_id) ) ) {
			client_id = ntohs(client_id);
			DEBUG(client_id << ": " << "In new client serving thread");
		} else {
			DEBUG("Client's message could not received properly");
			close_tcp_socket_and_thread(sock, client_id);
		}


		/* Calculate the video stream serving port:
		*******************************************/
		if ( ( client_id > 0 ) &&  ( client_id < ( MAX_CLIENT_ID + 1 ) ) ) {
			video_port = server_port + client_id;
			DEBUG(client_id << ": " << "Video pipeline port calculated" << std::endl <<
				client_id << ": " << "Will open video stream at port: " << video_port);
		} else {
			DEBUG(client_id << ": " << "Invalid client ID");
			close_tcp_socket_and_thread(sock, client_id);
		}


		/* Send video streaming port to client:
		***************************************/
		// Convert to network byte order
		video_port = htons(video_port);
		// Send video serving port value back to the client
		if ( ( sock->send(&video_port, sizeof(video_port) ) ) == ( sizeof(video_port) ) ) {
			DEBUG(client_id << ": " << "Video port successfully sent");
		} else {
			DEBUG(client_id << ": " << "Video streaming port could not be sent properly");
			close_tcp_socket_and_thread(sock, client_id);
		}


		/* Close the TCP socket:
		************************/
		DEBUG(client_id << ": " << "TCP socket closed");
		sock->close();


		/* Pass back to the calling function the received
		** client_id and video_port:
		*************************************************/
		// Convert to host byte order
		video_port = ntohs(video_port);
		*cid = client_id;
		*vport = video_port;


}
