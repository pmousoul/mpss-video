// Server video pipeline thread


#include "server_video_pipeline.h"


void* server_pipeline(void *thread_arg) {

	/* Variables passed from client-service-server's thread:
	********************************************************/
	/*
	 * Convert thread function argument to client_service
	 * video_pipeline_message structure
	 */
	video_pipeline_message *vp_message;
	vp_message = (video_pipeline_message *) thread_arg;

	const char *server_ip = vp_message->server_ip;
	uint16_t server_video_port = vp_message->server_video_port;
	bool *pipeline_force_stop = vp_message->pipeline_force_stop;
	bool *pipeline_state = vp_message->pipeline_state;
	thread_mutex_t *pipeline_state_mutex = vp_message->pipeline_state_mutex;
	thread_cond_t *pipeline_state_cond = vp_message->pipeline_state_cond;
	DEBUG(server_video_port << ": " << "Host received by server's video thread: " << server_ip << std::endl <<
		server_video_port << ": " << "Port received by server's video thread: " << server_video_port);


	/* Variables used by the video pipeline:
	****************************************/
	CustomData data;
	GstCaps *cap; // Used by the capsfilter element
	GstBus *bus;
	GstMessage *msg;
	GstStateChangeReturn ret;
	gboolean terminate = FALSE;
	uint64_t time_to_drop_late_buffer = 50000000; // 50 ms


	/* Initialize GStreamer:
	************************/
	gst_init (NULL, NULL);


	/* Create the elements:
	***********************/
	data.source = gst_element_factory_make ("udpsrc", "source");
	data.filter1 = gst_element_factory_make ("capsfilter", "filter1");
	data.filter2 = gst_element_factory_make ("rtpjitterbuffer", "filter2");
	data.filter3 = gst_element_factory_make ("rtph264depay", "filter3");
	data.filter4 = gst_element_factory_make ("h264parse", "filter4");
	data.filter5 = gst_element_factory_make ("avdec_h264", "filter5");
#ifdef DEBUG_FPS
	data.sink = gst_element_factory_make ("fpsdisplaysink", "sink");
#else
	data.filter6 = gst_element_factory_make ("jpegenc", "filter6");
	data.filter7 = gst_element_factory_make ("multipartmux", "filter7");
	data.sink = gst_element_factory_make ("udpsink", "sink");
#endif

	/* Create the cap element:
	**************************/
	cap = gst_caps_from_string ("application/x-rtp");


	/* Create the empty pipeline:
	*****************************/
	data.pipeline = gst_pipeline_new ("server-pipeline");
#ifdef DEBUG_FPS
	if (!data.pipeline || !data.source || !data.filter1 || !data.filter2 || !data.filter3 
		|| !data.filter4 || !data.filter5 || !data.sink) {
#else
	if (!data.pipeline || !data.source || !data.filter1 || !data.filter2 || !data.filter3 
		|| !data.filter4 || !data.filter5 || !data.filter6 || !data.filter7 || !data.sink) {
#endif
		DEBUG(server_video_port << ": " << "Not all elements could be created" << std::endl <<
			server_video_port << ": " << "UNLOCKING THE SERVER APPLICATION");
		// Unlock the server application
		unlock_server_app_if_pipeline_failed(pipeline_state, pipeline_state_mutex, pipeline_state_cond);
		DEBUG(server_video_port << ": " << "SERVER VIDEO THREAD CLOSED");
		thread_exit();
	}


	/* Build the pipeline:
	***********************/
#ifdef DEBUG_FPS
	gst_bin_add_many (GST_BIN (data.pipeline), data.source, data.filter1, data.filter2,
		data.filter3, data.filter4, data.filter5, data.sink, NULL);
	if (gst_element_link_many (data.source, data.filter1, data.filter2, data.filter3,
		data.filter4, data.filter5, data.sink, NULL) != TRUE) {
#else
	gst_bin_add_many (GST_BIN (data.pipeline), data.source, data.filter1, data.filter2,
		data.filter3, data.filter4, data.filter5, data.filter6, data.filter7, data.sink, NULL);
	if (gst_element_link_many (data.source, data.filter1, data.filter2, data.filter3,
		data.filter4, data.filter5, data.filter6, data.filter7, data.sink, NULL) != TRUE) {
#endif
		DEBUG(server_video_port << ": " << "Elements could not be linked");
		gst_object_unref (data.pipeline);
		DEBUG(server_video_port << ": " << "UNLOCKING THE SERVER APPLICATION");
		// Unlock the server application
		unlock_server_app_if_pipeline_failed(pipeline_state, pipeline_state_mutex, pipeline_state_cond);
		DEBUG(server_video_port << ": " << "SERVER VIDEO THREAD CLOSED");
		thread_exit();
	}


	/* Modify the elements' properties:
	***********************************/
	g_object_set (data.source, "port", server_video_port, NULL);
	g_object_set (data.filter1, "caps", cap, NULL);
#ifdef DEBUG_FPS
	g_object_set (data.sink, "sync", FALSE, NULL);
#else
	g_object_set (data.sink,
		"host", server_ip,
		"port", server_video_port + 2*MAX_CLIENT_ID,
		"sync", FALSE,
		"max-lateness", time_to_drop_late_buffer, NULL);
#endif


	/* Start playing:
	*****************/
	ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		DEBUG(server_video_port << ": " << "Unable to set the pipeline to the playing state");
		gst_object_unref (data.pipeline);
		DEBUG(server_video_port << ": " << "UNLOCKING THE SERVER APPLICATION");
		// Unlock the server application
		unlock_server_app_if_pipeline_failed(pipeline_state, pipeline_state_mutex, pipeline_state_cond);
		DEBUG(server_video_port << ": " << "SERVER VIDEO THREAD CLOSED");
		thread_exit();
	}


	/* Listen to the bus every 50 ms:
	*********************************/
	bus = gst_element_get_bus (data.pipeline);
	do {

		// Pipeline forced to stop from the server app
		thread_mutex_lock(pipeline_state_mutex);
		if ( *(pipeline_force_stop) == true ) terminate = TRUE;
		thread_mutex_unlock(pipeline_state_mutex);


		//msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, (GstMessageType) (GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
		msg = gst_bus_timed_pop_filtered (bus, 50000000, (GstMessageType) (GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS));


		// Parse message
		if (msg != NULL) {

			GError *err;
			gchar *debug_info;

			switch (GST_MESSAGE_TYPE (msg)) {

				case GST_MESSAGE_ERROR:
					gst_message_parse_error (msg, &err, &debug_info);
					DEBUG(server_video_port << ": " << "Error received from element " << GST_OBJECT_NAME (msg->src) << ": " << err->message);
					DEBUG(server_video_port << ": " << "Debugging information: " << (debug_info ? debug_info : "none"));
					g_clear_error (&err);
					g_free (debug_info);
					DEBUG(server_video_port << ": " << "UNLOCKING THE SERVER APPLICATION");
					// Unlock the server application
					unlock_server_app_if_pipeline_failed(pipeline_state, pipeline_state_mutex, pipeline_state_cond);
					terminate = TRUE;
					break;

				case GST_MESSAGE_EOS:
					DEBUG(server_video_port << ": " << "End-Of-Stream reached" << std::endl <<
						server_video_port << ": " << "UNLOCKING THE SERVER APPLICATION");
					// Unlock the server application
					unlock_server_app_if_pipeline_failed(pipeline_state, pipeline_state_mutex, pipeline_state_cond);
					terminate = TRUE;
					break;

				case GST_MESSAGE_STATE_CHANGED:
					// We are only interested in state-changed messages from the pipeline
					if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data.pipeline)) {
						GstState old_state, new_state, pending_state;
						gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
						DEBUG(server_video_port << ": " << "Pipeline state changed from " << gst_element_state_get_name (old_state)
							<< " to " << gst_element_state_get_name (new_state));
						// Mutex and conditional variable usage for checking pipeline state
						if ( !strcmp( (gst_element_state_get_name (new_state)), "READY" ) ){
							thread_mutex_lock(pipeline_state_mutex);
							*(pipeline_state) = true;
							thread_cond_signal(pipeline_state_cond);
							thread_mutex_unlock(pipeline_state_mutex);
						}
					}
					break;

				default:
					// We should not reach here
					DEBUG(server_video_port << ": " << "Unexpected message received" << std::endl <<
						server_video_port << ": " << "UNLOCKING THE SERVER APPLICATION");
					// Unlock the server application
					unlock_server_app_if_pipeline_failed(pipeline_state, pipeline_state_mutex, pipeline_state_cond);
					terminate = TRUE;
					break;

			}

			gst_message_unref (msg);

		}

	} while (!terminate);


	// Upadate the pipeline_state variable
	thread_mutex_lock(pipeline_state_mutex);
	*(pipeline_state) = false;
	thread_mutex_unlock(pipeline_state_mutex);


	// Free resources
	// GStreamer related
	gst_object_unref (bus);
	gst_element_set_state (data.pipeline, GST_STATE_NULL);
	gst_object_unref (data.pipeline);
	// pthread related
	DEBUG(server_video_port << ": " << "SERVER VIDEO THREAD CLOSED");
	thread_exit();

}
