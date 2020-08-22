// Server video pipeline thread related structures and functions


#ifndef SERVER_VIDEO_PIPELINE_H
#define SERVER_VIDEO_PIPELINE_H


/* Include files:
*****************/
// Standard Library:
#include <cstring> // For comparing strings
#include <stdint.h> // Use of fixed width int types
// Application Specific:
#include <gst/gst.h> // Gstreamer related
#include "Thread.h" // Use of pthreads for thread support
#include "video_pipeline_message.h"
// DEBUG Specific:
#include "Debug.h"



/* Data type definitions:
*************************/
struct CustomData {
	GstElement *pipeline;
	GstElement *source;
	GstElement *filter1;
	GstElement *filter2;
	GstElement *filter3;
	GstElement *filter4;
	GstElement *filter5;
	GstElement *filter6;
	GstElement *filter7;
	GstElement *sink;
};



/* Parameter Definition:
************************/
// Define max number of clients
#define MAX_CLIENT_ID 10



/* Function declarations:
*************************/
void *server_pipeline(void *thread_arg);
void *server_temp_pipeline(void *thread_arg);
void unlock_server_app_if_pipeline_failed(bool *pipeline_state, thread_mutex_t *pipeline_state_mutex, thread_cond_t *pipeline_state_cond);

#endif
