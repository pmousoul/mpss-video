// Helper function definitions


#include "client_video_pipeline.h"


/* Function definitions:
************************/

// Helper function definition used by the client_pipeline function:
void unlock_client_app_if_pipeline_failed(bool *pipeline_state, thread_mutex_t *pipeline_state_mutex, thread_cond_t *pipeline_state_cond) {

	thread_mutex_lock(pipeline_state_mutex);
	*(pipeline_state) = false;
	thread_cond_signal(pipeline_state_cond);
	thread_mutex_unlock(pipeline_state_mutex);

}
