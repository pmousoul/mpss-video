// Server client service thread


#include "server_client_service.h"


void *client_service(void *thread_arg){

	/*
	 * TODO:
	 * In the future save each client's service session
	 * in a log file in addition to printing the session
	 * to the stdout for debug purposes.
	 */


	/* Variables passed from server's main thread:
	**********************************************/
	/*
	 * Convert thread function argument to client_service
	 * video_pipeline_message structure
	 */
	client_service_message *cs_message;
	cs_message = (client_service_message *) thread_arg;
	TCPSocket *sock = cs_message->service_sock;
	const char *server_ip = cs_message->server_ip;
	uint16_t server_port = cs_message->server_port;

	/* Variables used by the server to service the client:
	******************************************************/
	uint16_t client_id;
	uint16_t video_port;


	try {

		/* Video port request service:
		******************************/
		/*
		* If video port request service fails, the TCP socket and the
		* current thread are closed.
		*/
		video_port_request_service(sock, &client_id, server_port, &video_port);


		/* Client-Server UDP communication used
		** to check video status and connectivity:
		******************************************/
		UDPSocket usock;
		SocketAddress uaddr(server_ip, video_port+MAX_CLIENT_ID, SocketAddress::UDP_SOCKET);
		usock.bind(uaddr);
		// Variable used to save the client's ip address
		SocketAddress uaddr_recv;
		DEBUG(client_id << ": " << "UDP SOCKET FOR CLIENT-SERVER COMMUNICATION CREATED");
		// Buffer used for client-server message exchange
		uint8_t message;
		// Changing the UDP socket to nonblocking
		usock.nonBlock();

		// Variables used to control the communication loop
		uint16_t comm_retries = 0;
		bool start_temp_pipeline = false;
		bool start_temp_pipeline_old = false;


		/* Main Loop:
		*************/
		DEBUG(client_id << ": " << "MAIN LOOP:");
		while ( 1 ) {

			// Communication sync variable
			bool first_comm_success = false;


			/* Inner Loop:
			**************/
			DEBUG(client_id << ": " << "INNER LOOP:");
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


				/* Variables passed to the video pipeline thread:
				*************************************************/
				// Variables for checking video pipeline's state
				bool pipeline_state = false;
				bool pipeline_force_stop = false;
				// Message structure passed to the video pipeline thread
				video_pipeline_message vp_message(
					server_ip,
					video_port,
					&pipeline_state,
					&pipeline_force_stop,
					&pipeline_state_mutex,
					&pipeline_state_cond);


				/* Create server-side video pipeline thread:
				********************************************/
				int rc;
				if ( start_temp_pipeline ) {
					rc = thread_create(&thread, &attr, server_temp_pipeline, (void *) &vp_message);
				} else {
					rc = thread_create(&thread, &attr, server_pipeline, (void *) &vp_message);
				}
				// Checking for proper thread creation
				if ( rc ) {
					DEBUG(client_id << ": " << "ERROR: could not create video stream thread" << std::endl <<
						client_id << ": " << "Return code from pthread_create() is " << rc);
					// Rerun the Main Loop
					break;
				}
				DEBUG(client_id << ": " << "Server-side video stream thread created");


				/* Wait until the video pipeline enters the READY state
				** or stop waiting when something goes wrong with the
				** the video pipeline:
				*******************************************************/
				thread_mutex_lock(&pipeline_state_mutex);
				while( !( (pipeline_state) ) ) {
					thread_cond_wait(&pipeline_state_cond, &pipeline_state_mutex);
					if ( !( (pipeline_state) ) ) {
						/*
						 * The pipeline state is false and this fact
						 * will be taken into account in video pipeline
						 * check done in the Communication Loop.
						 */
						break;
					}
					DEBUG(client_id << ": " << "VIDEO PIPELINE ENTERED THE READY STATE");
				}
				thread_mutex_unlock(&pipeline_state_mutex);


				/* Communication Loop:
				**********************/
				/*
				 * Sync timeout counter definition for measuring
				 * how many times there was an unsuccessfull
				 * server client UDP communication synchronization
				 */
				uint16_t sync_timeout = 0;
				DEBUG(client_id << ": " << "COMM LOOP:");
				while ( 1 ) {

					bool comm_error = false;


					// Check video pipeline state
					thread_mutex_lock(&pipeline_state_mutex);
					bool video_status = pipeline_state;
					thread_mutex_unlock(&pipeline_state_mutex);
					if ( ! ( video_status ) ) {
						DEBUG(client_id << ": " << "Server video pipeline stopped - trying to recover");
						// Rerun the Main Loop because the video
						// pipeline stopped working
						break;
					}


					// Clent-server communication and synchronization
					usleep(50000);
					// (01010101)_b == (85)_10 indicates "Server OK" in normal pipeline mode
					// (11110000)_b == (240)_10 indicates "Server started temp pipeline"
					// (00001111)_b == (15)_10 indicates "Server started new normal pipeline"
					message = 85;
					if ( ( comm_retries >= ( MAX_COMM_ATTEMPTS/5 ) ) &&
						( start_temp_pipeline_old == false ) &&
						( start_temp_pipeline == true ) ) message = 240;
					if ( ( comm_retries <= ( MAX_COMM_ATTEMPTS/10 ) ) &&
						( start_temp_pipeline_old == true ) &&
						( start_temp_pipeline == false ) ) message = 15;
					if ( ( usock.sendToNonBlock(&message, sizeof(message), uaddr_recv) ) != ( sizeof(message) ) ) {
						comm_error = true;
					}
					usleep(50000);
					if ( ( usock.recvFromNonBlock(&message, sizeof(message), uaddr_recv) ) != ( sizeof(message) ) ) {
						comm_error = true;
					}
					// (10101010)_b == (170)_10 indicates "Client OK" in normal pipeline mode
					// (00001111)_b == (15)_10 indicates "Client ACKed" in temp pipeline mode
					// (11110000)_b == (240)_10 indicates "Client ACKed" in new normal pipeline mode
					if ( ( message != 170 ) &&  ( message != 15 ) &&  ( message != 240 ) ) {
						comm_error = true;
					}
					if ( !comm_error && !first_comm_success ) {
							first_comm_success = true;
							DEBUG("INITIAL COMMUNICATION SYNC WITH THE CLIENT COMPLETED SUCCESSFULLY");
					} else if ( !first_comm_success ){
						sync_timeout++;
						//DEBUG("sync_timeout = " << (int)sync_timeout);
						if ( sync_timeout == MAX_COMM_ATTEMPTS*3 ) {
							DEBUG(client_id << ": " << "CLIENT-SERVER SYNC IMPOSSIBLE");
							thread_mutex_lock(&pipeline_state_mutex);
							pipeline_force_stop = true;
							thread_mutex_unlock(&pipeline_state_mutex);
							usock.close();
							DEBUG(client_id << ": " << "Forced video pipeline to stop" << std::endl <<
								client_id << ": " << "UDP socket closed");
							thread_mutex_cond_destroy(&pipeline_state_mutex, &pipeline_state_cond);
							DEBUG(client_id << ": " << "Thread closed");
							thread_exit();
						}
					}
					if ( first_comm_success ) {
						if ( comm_error ) {
							comm_retries++;
							if ( comm_retries%5 == 0 )
								DEBUG(client_id << ": " << "CLIENT-SERVER COMMUNICATION FAILED " << comm_retries << " TIME(S)");
						} else {
							if (comm_retries > 0)
								comm_retries--;
							else
								comm_retries = 0;
						}
					} else {
						continue;
					}


					// Start a temporary pipeline if communication
					// retries exceed MAX_COMM_ATTEMPTS/5
					if ( ( comm_retries >= ( MAX_COMM_ATTEMPTS/5 ) ) && ( start_temp_pipeline == false ) ) {
						DEBUG(client_id << ": " << "CLIENT-SERVER COMMUNICATION TEMPORARILY LOST" << std::endl <<
							client_id << ": " << "STARTING A TEMP VIDEO PIPELINE UNTIL RECONNECTION IS ESTABLISHED");
						thread_mutex_lock(&pipeline_state_mutex);
						pipeline_force_stop = true;
						thread_mutex_unlock(&pipeline_state_mutex);

						start_temp_pipeline_old = start_temp_pipeline;
						start_temp_pipeline = true;

						DEBUG(client_id << ": " << "FORCED NORMAL VIDEO PIPELINE TO STOP");
						// Rerun the Main Loop
						break;
					}


					// Close the temporary pipeline and start a normal one
					// if communication retries fall bellow MAX_COMM_ATTEMPTS/10
					if ( ( comm_retries <= ( MAX_COMM_ATTEMPTS/10 ) ) && ( start_temp_pipeline == true ) ) {
						DEBUG(client_id << ": " << "CLIENT-SERVER RECONNECTION ESTABLISHED" << std::endl <<
							client_id << ": " << "STARTING NORMAL VIDEO PIPELINE");
						thread_mutex_lock(&pipeline_state_mutex);
						pipeline_force_stop = true;
						thread_mutex_unlock(&pipeline_state_mutex);

						start_temp_pipeline_old = start_temp_pipeline;
						start_temp_pipeline = false;

						DEBUG(client_id << ": " << "FORCED TEMP VIDEO PIPELINE TO STOP");
						// Rerun the Main Loop
						break;
					}


					// Maximum number of communication retries check
					if ( comm_retries == MAX_COMM_ATTEMPTS ) {

						DEBUG(client_id << ": " << "CLIENT-SERVER COMMUNICATION LOST");
						thread_mutex_lock(&pipeline_state_mutex);
						pipeline_force_stop = true;
						thread_mutex_unlock(&pipeline_state_mutex);
						usock.close();
						DEBUG(client_id << ": " << "Forced video pipeline to stop" << std::endl <<
							client_id << ": " << "UDP socket closed");

						thread_mutex_cond_destroy(&pipeline_state_mutex, &pipeline_state_cond);
						DEBUG(client_id << ": " << "Thread closed");
						thread_exit();
					}


				}


				usleep(100000);
				break;


			}


		}


	} catch (SocketException &e) {

		DEBUG(client_id << ": " << "SOCKET EXCEPTION OCCURED");
		DEBUG(client_id << ": " << e.what());

	}


	/* Close remaining open socket(s) in case of exception:
	*******************************************************/
	sock->close();

}
