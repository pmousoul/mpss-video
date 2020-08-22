// Client video pipeline thread


#include "client_video_pipeline.h"


void *client_pipeline(void *thread_arg) {

	/* Variables passed from client's main thread:
	**********************************************/
	/* 
	 * Convert thread function argument to video_pipeline_message
	 * structure
	 */
	video_pipeline_message *vp_message;
	vp_message = (video_pipeline_message *) thread_arg;

	const char *server_ip = vp_message->server_ip;
	uint16_t server_video_port = vp_message->server_video_port;
	bool *pipeline_state = vp_message->pipeline_state;
	bool *pipeline_force_stop = vp_message->pipeline_force_stop;
	thread_mutex_t *pipeline_state_mutex = vp_message->pipeline_state_mutex;
	thread_cond_t *pipeline_state_cond = vp_message->pipeline_state_cond;
	const char *video_device = vp_message->video_device;
	uint16_t video_width = vp_message->video_width;
	uint16_t video_height = vp_message->video_height;
	uint8_t video_framerate = vp_message->video_framerate;
	DEBUG("IP received by client's video thread: " << server_ip << std::endl <<
		"Port received by client's video thread: " << server_video_port);


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


#ifdef H264C

	/* Create the elements:
	***********************/
	data.source = gst_element_factory_make ("v4l2src", "source");
	data.filter1 = gst_element_factory_make ("capsfilter", "filter1");
	data.filter2 = gst_element_factory_make ("x264enc", "filter2");
	data.filter3 = gst_element_factory_make ("h264parse", "filter3");
	data.filter4 = gst_element_factory_make ("rtph264pay", "filter4");
	data.sink = gst_element_factory_make ("udpsink", "sink");


	/* Create the cap element:
	**************************/
	cap = gst_caps_new_simple (
		"video/x-raw",
		"width", G_TYPE_INT, video_width,
		"height", G_TYPE_INT, video_height,
		"framerate", GST_TYPE_FRACTION, video_framerate, 1, NULL);


	/* Create the empty pipeline:
	*****************************/
	data.pipeline = gst_pipeline_new ("client-pipeline");
	if (!data.pipeline || !data.source || !data.filter1 || !data.filter2 || !data.filter3 || !data.filter4 || !data.sink) {
		DEBUG("Not all elements could be created" << std::endl <<
			"UNLOCKING THE CLIENT APPLICATION");
		// Unlock the client application
		unlock_client_app_if_pipeline_failed(pipeline_state, pipeline_state_mutex, pipeline_state_cond);
		DEBUG("VIDEO THREAD CLOSED");
		thread_exit();
	}


	/* Build the pipeline:
	**********************/
	gst_bin_add_many (GST_BIN (data.pipeline), data.source, data.filter1, data.filter2, data.filter3, data.filter4, data.sink, NULL);
	if (gst_element_link_many (data.source, data.filter1, data.filter2, data.filter3, data.filter4, data.sink, NULL) != TRUE) {
		DEBUG("Elements could not be linked");
		gst_object_unref (data.pipeline);
		DEBUG("UNLOCKING THE CLIENT APPLICATION");
		// Unlock the client application
		unlock_client_app_if_pipeline_failed(pipeline_state, pipeline_state_mutex, pipeline_state_cond);
		DEBUG("VIDEO THREAD CLOSED");
		thread_exit();
	}


	/* Modify the elements' properties:
	***********************************/
	g_object_set (data.source, "device", video_device, NULL);
	g_object_set (data.filter1,"caps", cap, NULL);
	g_object_set (data.sink,
		"host", server_ip,
		"port", server_video_port,
		"sync", FALSE,
		"max-lateness", time_to_drop_late_buffer, NULL);


#elif H264N

	/* Create the elements:
	***********************/
	#ifdef C920
	data.source = gst_element_factory_make ("v4l2src", "source");
	#elif RPI
	data.source = gst_element_factory_make ("rpicamsrc", "source");
	#endif
	data.filter1 = gst_element_factory_make ("capsfilter", "filter1");
	data.filter2 = gst_element_factory_make ("h264parse", "filter2");
	data.filter3 = gst_element_factory_make ("rtph264pay", "filter3");
	data.sink = gst_element_factory_make ("udpsink", "sink");


	/* Create the cap element:
	**************************/
	cap = gst_caps_new_simple (
		"video/x-h264",
		"width", G_TYPE_INT, video_width,
		"height", G_TYPE_INT, video_height,
		"framerate", GST_TYPE_FRACTION, video_framerate, 1, NULL);


	/* Create the empty pipeline:
	*****************************/
	data.pipeline = gst_pipeline_new ("client-pipeline");
	if (!data.pipeline || !data.source || !data.filter1 || !data.filter2 || !data.filter3 || !data.sink) {
		DEBUG("Not all elements could be created" << std::endl <<
			"UNLOCKING THE CLIENT APPLICATION");
		// Unlock the client application
		unlock_client_app_if_pipeline_failed(pipeline_state, pipeline_state_mutex, pipeline_state_cond);
		DEBUG("VIDEO THREAD CLOSED");
		thread_exit();
	}


	/* Build the pipeline:
	**********************/
	gst_bin_add_many (GST_BIN (data.pipeline), data.source, data.filter1, data.filter2, data.filter3, data.sink, NULL);
	if (gst_element_link_many (data.source, data.filter1, data.filter2, data.filter3, data.sink, NULL) != TRUE) {
		DEBUG("Elements could not be linked");
		gst_object_unref (data.pipeline);
		DEBUG("UNLOCKING THE CLIENT APPLICATION");
		// Unlock the client application
		unlock_client_app_if_pipeline_failed(pipeline_state, pipeline_state_mutex, pipeline_state_cond);
		DEBUG("VIDEO THREAD CLOSED");
		thread_exit();
	}


	/* Modify the elements' properties:
	***********************************/
	#ifdef C920
	g_object_set (data.source,
		"device", video_device, NULL);
	#elif RPI
	g_object_set (data.source,
		"preview", 0,
		"rotation", 180, NULL);
		//"bitrate", 0,
		//"quantisation-parameter", 28, NULL); // Values for quantisation-parameter should vary between 16-42 (or 10-40 as stated in rpicamsrc element).
	#endif
	g_object_set (data.filter1,"caps", cap, NULL);
	g_object_set (data.sink,
		"host", server_ip,
		"port", server_video_port,
		"sync", FALSE,
		"max-lateness", time_to_drop_late_buffer, NULL);


#endif


	/* Start playing:
	*****************/
	ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		DEBUG("Unable to set the pipeline to the playing state");
		gst_object_unref (data.pipeline);
		DEBUG("UNLOCKING THE CLIENT APPLICATION");
		// Unlock the client application
		unlock_client_app_if_pipeline_failed(pipeline_state, pipeline_state_mutex, pipeline_state_cond);
		DEBUG("VIDEO THREAD CLOSED");
		thread_exit();
	}


	/* Listen to the bus:
	*********************/
	bus = gst_element_get_bus (data.pipeline);
	do {

		// Pipeline forced to stop from the client app
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
					DEBUG("Error received from element " << GST_OBJECT_NAME (msg->src) << ": " << err->message);
					DEBUG("Debugging information: " << (debug_info ? debug_info : "none"));
					g_clear_error (&err);
					g_free (debug_info);
					DEBUG("UNLOCKING THE CLIENT APPLICATION");
					// Unlock the client application
					unlock_client_app_if_pipeline_failed(pipeline_state, pipeline_state_mutex, pipeline_state_cond);
					terminate = TRUE;
					break;

				case GST_MESSAGE_EOS:
					DEBUG("End-Of-Stream reached" << std::endl <<
						"UNLOCKING THE CLIENT APPLICATION");
					// Unlock the client application
					unlock_client_app_if_pipeline_failed(pipeline_state, pipeline_state_mutex, pipeline_state_cond);
					terminate = TRUE;
					break;

				case GST_MESSAGE_STATE_CHANGED:
					// We are only interested in state-changed messages from the pipeline
					if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data.pipeline)) {
						GstState old_state, new_state, pending_state;
						gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
						DEBUG("Pipeline state changed from " << gst_element_state_get_name (old_state)
							<< " to " << gst_element_state_get_name (new_state));
						// Mutex and conditional variable usage for checking pipeline state
						if ( !( strcmp( (gst_element_state_get_name (new_state)), "PLAYING" ) ) ) {
							thread_mutex_lock(pipeline_state_mutex);
							*(pipeline_state) = true;
							thread_cond_signal(pipeline_state_cond);
							thread_mutex_unlock(pipeline_state_mutex);
						}
					}
					break;

				default:
					// We should never reach here
					DEBUG("Unexpected message received" << std::endl <<
						"UNLOCKING THE CLIENT APPLICATION");
					// Unlock the client application
					unlock_client_app_if_pipeline_failed(pipeline_state, pipeline_state_mutex, pipeline_state_cond);
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
	DEBUG("VIDEO THREAD CLOSED");
	thread_exit();


}
