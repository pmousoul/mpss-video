// Message passed to the video pipeline


#ifndef VIDEO_PIPELINE_MESSAGE_H
#define VIDEO_PIPELINE_MESSAGE_H


// Video pipeline message class
struct video_pipeline_message {

	/* Class variables:
	*******************/
	const char *server_ip;
	uint16_t server_video_port;
	bool *pipeline_state;
	bool *pipeline_force_stop;
	thread_mutex_t *pipeline_state_mutex;
	thread_cond_t *pipeline_state_cond;

	// Client's camera parameters
#ifdef CLIENT
	const char *video_device;
	uint16_t video_width;
	uint16_t video_height;
	uint8_t video_framerate;
#endif


	/* Class constructors:
	**********************/
	video_pipeline_message(){} // Default constructor

#ifdef CLIENT
	video_pipeline_message(
		const char *ip,
		uint16_t port,
		bool *state,
		bool *force_stop,
		thread_mutex_t *mutex,
		thread_cond_t *cond,
		const char *vd,
		uint16_t vw,
		uint16_t vh,
		uint8_t vf):
		
		server_ip(ip),
		server_video_port(port),
		pipeline_state(state),
		pipeline_force_stop(force_stop),
		pipeline_state_mutex(mutex),
		pipeline_state_cond(cond),
		video_device(vd),
		video_width(vw),
		video_height(vh),
		video_framerate(vf) {}
#else
	video_pipeline_message(
		const char *ip,
		uint16_t port,
		bool *state,
		bool *force_stop,
		thread_mutex_t *mutex,
		thread_cond_t *cond):

		server_ip(ip),
		server_video_port(port),
		pipeline_state(state),
		pipeline_force_stop(force_stop),
		pipeline_state_mutex(mutex),
		pipeline_state_cond(cond) {}
#endif

};

#endif
