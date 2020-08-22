// Client video pipeline thread related structures and functions


#ifndef CLIENT_VIDEO_PIPELINE_H
#define CLIENT_VIDEO_PIPELINE_H


/* Include files:
*****************/
// Standard Library:
#include <cstring> // For strcmp()
#include <stdint.h> // Use of fixed width int types
// Application Specific:
#include <gst/gst.h> // Gstreamer related
#include "Thread.h" // Use of pthreads for thread support
#include "video_pipeline_message.h"
// DEBUG Specific:
#include "Debug.h"



/* Data type definitions:
*************************/
// Gstreamer element holder structure:
struct CustomData {
	GstElement *pipeline;
	GstElement *source;
	GstElement *filter1;
	GstElement *filter2;
	GstElement *filter3;
#ifdef H264C
	GstElement *filter4;
#endif
	GstElement *sink;
};



/* Function declarations:
*************************/
// Client_pipeline function declaration - to be used by the client:
void *client_pipeline(void *thread_arg);
// Helper function - to be used by the client_pipeline function:
void unlock_client_app_if_pipeline_failed(bool *pipeline_state, thread_mutex_t *pipeline_state_mutex, thread_cond_t *pipeline_state_cond);

#endif
