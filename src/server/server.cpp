// Server application


#include "server.h"


int main(int argc, char *argv[]) {

	/* Check the number of arguments:
	*********************************/
	if (argc != 3) {
		DEBUG("Usage: ./Server <server host> <listening port>" << std::endl <<
			"Wrong number of arguments");
		return 0;
	}


	/* Variables used by the server:
	********************************/
	const char *server_ip = argv[1];
	uint16_t server_port = atoi(argv[2]);


	/* Main Loop - loop forever:
	****************************/
	DEBUG("SERVER STARTED:");
	while ( 1 ) {

		try {

			/* Make a socket to listen for client connections:
			**************************************************/
			/*
			 * MAX_CLIENT_ID is used as the maximum queue length
			 * for outstanding connection requests
			 */
			TCPServerSocket servSock(server_port, MAX_CLIENT_ID);
			DEBUG("Server is listening for clients at port: " << server_port);


			// Loop until an exception occurs
			while ( 1 ) {

				/* Repeatedly accept connections:
				*********************************/
				TCPSocket *sock = servSock.accept();
				DEBUG("\n\nServer accepted a new connection");


				/*
				 * TODO: A check should be done to register clients by
				 * their ip (e.g.):
				 * DEBUG(sock->getForeignAddress().getAddress());
				 * if the IP is not legal then the loop continues
				 * without opening a thread to serve the illegal
				 * client. This issue could also be resolved by a
				 * dedicated AP that will permit access to the 
				 * local network only to legal devices (e.g. usign 
				 * security, MAC address registration etc.).
				 */


				/* Define thread for client service:
				************************************/
				thread_t thread;
				thread_attr_t attr;
				thread_init(&thread, &attr);


				/* Message structure to pass to the client service thread:
				**********************************************************/
				client_service_message cs_message(sock, server_ip, server_port);


				/* Create client service thread:
				********************************/
				int rc = thread_create(&thread, &attr, client_service, (void *) &cs_message);
				// Checking for proper thread creation
				if ( rc ) {
					DEBUG("ERROR: could not create new client service thread" << std::endl <<
						"Return code from pthread_create() is " << rc);
					/*
					 * Close and delete the current TCPSocket
					 * The server will keep running
					 */
					sock->close();
					DEBUG("TCP socket for current client closed");
				}


				// We don't have to worry about the thread resources in case
				// of termination because the thread is created as DETACHED


			}


		} catch (SocketException &e) {

			DEBUG("SOCKET EXCEPTION OCCURED");
			DEBUG(e.what());

		}


		usleep(100000);
		DEBUG("SERVER RESTARTED\n\n");


	}


	// Unexpected behavior occured:
	//-----------------------------
	DEBUG("UNEXPECTED BEHAVIOR OCCURED" << std::endl <<
		"Server stopped listening for client requests" << std::endl <<
		"Waiting for already opened threads to finish their work");
	thread_exit();
	return 0;


}
