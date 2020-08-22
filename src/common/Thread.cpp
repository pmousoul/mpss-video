// pthread application-specific wrapper


#include "Thread.h"


/* Function definitions:
************************/

void thread_init(thread_t *thread_p, thread_attr_t *attr_p, int detachstate){

	/* Initialize thread attribute
	 * Many attributes can be set on a single attribute object
	 */
	pthread_attr_init(attr_p);

	/* Get thread default stacksize attribute on current machine
	 * This is temporary; it will be removed once the stacksize is defined
	 */
	size_t stacksize;
	pthread_attr_getstacksize (attr_p, &stacksize);
	DEBUG("Default thread stacksize on this machine is: " << stacksize << " bytes");
	/* Example code to set stacksize in the feature for portability
	 * if (default_stack_size < MIN_REQ_SSIZE) {stacksize = sizeof(double)*N*N+MEGEXTRA;}
	 * pthread_attr_setstacksize (attr_p, stacksize);
	 */

	// Set thread detached attribute
	pthread_attr_setdetachstate(attr_p, detachstate);

}

void thread_mutex_cond_init(thread_mutex_t *mutex, thread_cond_t *cond){

	// Initialize mutex and condition variable objects
	pthread_mutex_init(mutex, NULL);
	pthread_cond_init(cond, NULL);

}

int thread_create(thread_t *thread_p, thread_attr_t *attr_p, void *(*routine) (void *), void *arg){

	int rc = pthread_create(thread_p, attr_p, routine, arg);

	// Free uneeded thread resources
	pthread_attr_destroy(attr_p);

	return rc;

}

void thread_mutex_lock(thread_mutex_t *mutex){

	pthread_mutex_lock(mutex);

}

void thread_mutex_unlock(thread_mutex_t *mutex){

	pthread_mutex_unlock(mutex);

}

void thread_cond_wait(thread_cond_t *cond, thread_mutex_t *mutex){

	pthread_cond_wait(cond, mutex);

}

void thread_cond_signal(thread_cond_t *cond){

	pthread_cond_signal(cond);

}

void thread_mutex_cond_destroy(thread_mutex_t *mutex, thread_cond_t *cond){

	pthread_mutex_destroy(mutex);
	pthread_cond_destroy(cond);

}

void thread_exit(){

	pthread_exit(NULL);

}
