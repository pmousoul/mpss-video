// pthread application-specific wrapper


#ifndef THREAD_H
#define THREAD_H


/* Include files:
*****************/
#include <pthread.h>
#include "Debug.h" // "Debug.h" is temporarily included until thread stacksize is set



/* Data type definitions:
**************************/
typedef pthread_t thread_t;
typedef pthread_attr_t thread_attr_t;
typedef pthread_mutex_t thread_mutex_t;
typedef pthread_cond_t thread_cond_t;



/* Function declarations:
*************************/
void thread_init(thread_t *thread_p, thread_attr_t *attr_p, int detachstate = PTHREAD_CREATE_DETACHED);
void thread_mutex_cond_init(thread_mutex_t *mutex, thread_cond_t *cond);
int thread_create(thread_t *thread_p, thread_attr_t *attr_p, void *(*routine) (void *), void *arg);
void thread_mutex_lock(thread_mutex_t *mutex);
void thread_mutex_unlock(thread_mutex_t *mutex);
void thread_cond_wait(thread_cond_t *cond, thread_mutex_t *mutex);
void thread_cond_signal(thread_cond_t *cond);
void thread_mutex_cond_destroy(thread_mutex_t *mutex, thread_cond_t *cond);
void thread_exit();

#endif
