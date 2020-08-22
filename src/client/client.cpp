// Client application


#include "client.h"


int main(int argc, char *argv[]) {

	/* Check the number of arguments:
	*********************************/
	// TODO: In the future, the arguments should be read from a .config file
	// Arguments assumed to be correct type by definition
	if ( argc < 8 ) {
		DEBUG("Wrong number of arguments" << std::endl <<
			"Usage:" << std::endl <<
			"./Client <server ip> <server port> <client id> <client video device> <video width> <video height> <video framerate>");
		return 0;
	}


	/* Variables used by the client:
	********************************/
	// Variables used to connect to the server
	const char *server_ip = argv[1];
	uint16_t server_port = atoi(argv[2]);
	uint8_t client_id = atoi(argv[3]);
	// Variables used to construct the client video pipeline
	const char *video_device = argv[4];
	uint16_t video_width = atoi(argv[5]);
	uint16_t video_height = atoi(argv[6]);
	uint8_t video_framerate = atoi(argv[7]);

	/* Main Loop variables:
	***********************/
	uint16_t video_port;
	bool reconnect = true;
	uint16_t comm_retries = 0;
	bool server_started_temp_pipe = false;


	/* Main Loop - loop forever:
	****************************/
	/*
	 * The MAIN LOOP exists to reset the application upon exceptions.
	 * Because of the scope, all the variables in the INNER LOOP
	 * will be destroyed and re-defined.
	 */
	DEBUG("MAIN LOOP:");
	while ( 1 ) {

		// Communication sync variable
		bool first_comm_success = false;


		try {

			/* The client connects to the server using TCP and
			** requests a port number to start video streaming:
			***************************************************/
			if ( reconnect ) {
				video_port = video_port_request(server_ip, server_port, client_id);
				DEBUG("VIDEO STREAMING PORT RECEIVED FROM SERVER");
				reconnect = false;
			}


			/* Inner Loop:
			**************/
			DEBUG("INNER LOOP:");
			while ( 1 ) {

				/* Define-initialize thread, attribute, mutex,
				** and condition variable for video streaming:
				**********************************************/
				thread_t thread;
				thread_attr_t attr;
				thread_mutex_t pipeline_state_mutex;
				thread_cond_t pipeline_state_cond;
				thread_init(&thread, &attr);
				thread_mutex_cond_init(&pipeline_state_mutex, &pipeline_state_cond);


				/* Variables to pass to the video pipeline thread:
				**************************************************/
				// Variables for checking video pipeline's state
				bool pipeline_state = false;
				bool pipeline_force_stop = false;

				// Message structure to pass to the video pipeline thread
				video_pipeline_message vp_message(
					server_ip,
					video_port,
					&pipeline_state,
					&pipeline_force_stop,
					&pipeline_state_mutex,
					&pipeline_state_cond,
					video_device,
					video_width,
					video_height,
					video_framerate);


				if ( !( server_started_temp_pipe ) ) {

					/* Create client-side video pipeline thread:
					********************************************/
					int rc = thread_create(&thread, &attr, client_pipeline, (void *) &vp_message);
					// Checking for proper thread creation
					if ( rc ) {
						DEBUG("ERROR: could not create new video stream thread" << std::endl <<
							"Return code from pthread_create() is " << rc);
						// Clear thread resources and exit the INNER LOOP
						thread_mutex_cond_destroy(&pipeline_state_mutex, &pipeline_state_cond);
						DEBUG("Released thread resources");
						// Rerun the Main Loop
						break;
					}
					DEBUG("CLIENT-SIDE VIDEO STREAM THREAD CREATED");


					/* Wait until the video pipeline enters the PLAYING state
					** or stop waiting when something goes wrong with the
					** building of the video pipeline:
					*********************************************************/
					thread_mutex_lock(&pipeline_state_mutex);
					while( !(pipeline_state) ) {
						DEBUG("Waiting for video pipeline to enter the PLAYING state");
						thread_cond_wait(&pipeline_state_cond, &pipeline_state_mutex);
						/*
						 * Get out of the loop if something went wrong with
						 * the building of the video pipeline.
						 * The pipeline state is false and this fact
						 * will be taken into account in video pipeline
						 * check done in the Communication Loop.
						 */
						if ( !(pipeline_state) ) break;
						DEBUG("VIDEO PIPELINE ENTERED THE PLAYING STATE");
					}
					thread_mutex_unlock(&pipeline_state_mutex);


				}


				/* Client-Server communication used to check video status
				** and connectivity:
				*********************************************************/
				// Define a UDP Socket
				UDPSocket usock;
				// Server's ip address
				SocketAddress uaddr_server(server_ip, video_port+MAX_CLIENT_ID, SocketAddress::UDP_SOCKET);
				// Create the UDP socket
				usock.create(uaddr_server);
				// Address used to receive
				SocketAddress uaddr_recv;
				DEBUG("UDP SOCKET FOR CLIENT-SERVER COMMUNICATION CREATED");
				// Buffers used for client-server message exchange
				uint8_t message_rcv;
				uint8_t message_snd;
				// Changing the UDP socket to non-blocking
				usock.nonBlock();


				/* Communication loop:
				**********************/
				/*
				 * Sync timeout counter definition for measuring
				 * how many times there was an unsuccessfull
				 * server client UDP communication synchronization
				 */
				uint16_t sync_timeout = 0;
				DEBUG("COMM LOOP:");
				while ( 1 ) {

					bool comm_error = false;


					// Check video pipeline state
					if ( !( pipeline_force_stop ) && !( server_started_temp_pipe ) ) {

						thread_mutex_lock(&pipeline_state_mutex);
						bool video_status = pipeline_state;
						thread_mutex_unlock(&pipeline_state_mutex);
						if ( !(video_status) ) {
							usock.close();
							DEBUG("CLIENT VIDEO PIPELINE STOPPED UNEXPECTEDTLY" << std::endl <<
								"UDP SOCKET CLOSED");
							// Rerun the Main Loop
							break;
						}


					}


					/* Clent-server communication and synchronization:
					**************************************************/
					usleep(50000);
					if ( ( usock.recvFromNonBlock(&message_rcv, sizeof(message_rcv), uaddr_recv) ) != ( sizeof(message_rcv) ) ) {
						comm_error = true;
					}
					// (01010101)_b == (85)_10 indicates "Server OK" in normal pipeline mode
					// (11110000)_b == (240)_10 indicates "Server started temp pipeline"
					// (00001111)_b == (15)_10 indicates "Server started new normal pipeline"
					if ( ( message_rcv != 85 ) &&  ( message_rcv != 240 ) &&  ( message_rcv != 15 ) ) {
						comm_error = true;
					}
					usleep(50000);
					// (10101010)_b == (170)_10 indicates "Client OK" in normal pipeline mode
					// (00001111)_b == (15)_10 indicates "Client ACKed" in temp pipeline mode
					// (11110000)_b == (240)_10 indicates "Client ACKed" in new normal pipeline mode
					if ( message_rcv == 85 ) message_snd = 170;
					if ( message_rcv == 240 ) message_snd = 15;
					if ( message_rcv == 15 ) message_snd = 240;
					if ( ( usock.sendToNonBlock(&message_snd, sizeof(message_snd), uaddr_server) ) != ( sizeof(message_snd) ) ) {
						comm_error = true;
					}
					if ( !comm_error ) {
						if ( uaddr_server.getAddress() != uaddr_recv.getAddress() ) {
							comm_error = true;
							DEBUG("RECEIVED UDP PACKET FROM UNEXPECTED SOURCE");
						} else if ( !first_comm_success ) {
							first_comm_success = true;
							DEBUG("INITIAL COMMUNICATION SYNC WITH THE SERVER COMPLETED SUCCESSFULLY");
						}
					}
					if ( first_comm_success ) {
						if ( comm_error ) {
							comm_retries++;
							if ( comm_retries%5 == 0 )
								DEBUG("CLIENT-SERVER COMMUNICATION FAILED " << comm_retries << " TIME(S)");
						} else {
							if (comm_retries > 0)
								comm_retries--;
							else
								comm_retries = 0;
						}
					} else {
						sync_timeout++;
						//DEBUG("sync_timeout = " << (int)sync_timeout);
						if ( sync_timeout == MAX_COMM_ATTEMPTS*3 ) {
							DEBUG("FAIL STATE COMMUNICATION WITH THE SERVER COMPLETED SUCCESSFULLY" << std::endl <<
							"CLIENT-SERVER COMMUNICATION LOST");
							thread_mutex_lock(&pipeline_state_mutex);
							pipeline_force_stop = true;
							thread_mutex_unlock(&pipeline_state_mutex);
							usock.close();
							DEBUG("FORCED VIDEO PIPELINE TO STOP" << std::endl <<
								"UDP SOCKET CLOSED");
							reconnect = true;
							// Rerun the Main loop
							break;
						}
						continue;
					}


					// Check if server started a temp pipeline
					// (11110000)_b == (240)_10 indicates "Server started temp pipeline"
					if ( ( message_rcv == 240 ) && ( !server_started_temp_pipe ) ) {
						server_started_temp_pipe = true;
						DEBUG("SERVER STARTED TEMP PIPELINE");
						thread_mutex_lock(&pipeline_state_mutex);
						pipeline_force_stop = true;
						thread_mutex_unlock(&pipeline_state_mutex);
						DEBUG("FORCED VIDEO PIPELINE TO STOP");
						usock.close();
						// Rerun the Main Loop
						break;
					}


					// Check if server started a normal pipeline
					// (00001111)_b == (15)_10 indicates "Server started normal pipeline"
					if ( ( message_rcv == 15 ) && ( server_started_temp_pipe ) ) {
						server_started_temp_pipe = false;
						DEBUG("SERVER STARTED NORMAL PIPELINE" << std::endl <<
						"STARTING CLIENT PIPELINE");
						usock.close();
						// Rerun the Main Loop
						break;
					}


					// Fail-state communication with the server check
					// Connection with the server failed for more than
					// 15sec. Server disconnects clients which are
					// unresponsive for more than 10sec. 
					if ( comm_retries == 3*MAX_COMM_ATTEMPTS ) {
						DEBUG("FAIL STATE COMMUNICATION WITH THE SERVER COMPLETED SUCCESSFULLY" << std::endl <<
							"CLIENT-SERVER COMMUNICATION LOST");
						thread_mutex_lock(&pipeline_state_mutex);
						pipeline_force_stop = true;
						thread_mutex_unlock(&pipeline_state_mutex);
						usock.close();
						DEBUG("FORCED VIDEO PIPELINE TO STOP" << std::endl <<
							"UDP SOCKET CLOSED");
						reconnect = true;
						// Rerun the Main loop
						break;
					}


				}


				/* Free uneeded thread resources and exit the inner loop:
				*********************************************************/
				thread_mutex_cond_destroy(&pipeline_state_mutex, &pipeline_state_cond);
				DEBUG("COMM LOOP ENDED");
				break;


			} // Inner Loop ends here


		} catch ( SocketException &e ) {

			DEBUG("SOCKET EXCEPTION OCCURED");
			DEBUG(e.what());


		}


		usleep(100000);
		DEBUG("INNER LOOP ENDED" << std::endl <<
			"RE-RUNNING CLIENT'S MAIN LOOP\n\n");


	} // Main Loop ends here


	/* We should never reach this point:
	************************************/
	DEBUG("MAIN LOOP ENDED" << std::endl <<
		"UNEXPECTED BEHAVIOR OCCURED");
	return 0;

}
