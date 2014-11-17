#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#include "thread_pool.h"

/**
 *  @struct threadpool_task
 *  @brief the work struct
 *
 *  Feel free to make any modifications you want to the function prototypes and structs
 *
 *  @var function Pointer to the function that will perform the task.
 *  @var argument Argument to be passed to the function.
 */

#define MAX_THREADS 20
#define STANDBY_SIZE 8
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

typedef struct {
	void (*function)(void *);
	void *argument;
} pool_task_t;


struct pool_t {
	pthread_mutex_t lock;
	pthread_cond_t notify;
	pthread_t *threads;
	pool_task_t* queue;
	int thread_count;
	int active_threads;
	int task_queue_size_limit;
	int queue_length;
	int shutdown;
};

static void *thread_do_work(void *pool);

/*
 * Add a task to the worker queue
 *
 */

int enqueue(pool_t *pool, void (*function)(void *), void *argument) {


	

	if (pool->queue_length == pool->task_queue_size_limit){
	
		
		return 0; 
	}

	pool->queue[pool->queue_length].argument = argument;
	pool->queue[pool->queue_length].function = function;
	
	pool->queue_length++;   



	
	 return 1;
}

/*
 * Remove task from worker queue
 *
 */

pool_task_t dequeue(pool_t *pool) {
	pool_task_t ret = pool->queue[0];
	
	int i;
	for(i=0; i < pool->queue_length-1; i++){
		pool->queue[i] = pool->queue[i+1];
	}
	
	pool->queue[pool->queue_length-1].argument = NULL;
	pool->queue[pool->queue_length-1].function = NULL;
	pool->queue_length--;

	return ret;
}


/*
 * Create a threadpool, initialize variables, etc
 *
 */
pool_t *pool_create(int queue_size, int num_threads)
{
		pool_t* new_threadpool = (pool_t*)malloc(sizeof(pool_t));
		new_threadpool->task_queue_size_limit = queue_size;
		new_threadpool->thread_count = num_threads;
		new_threadpool->active_threads = 0;
		new_threadpool->shutdown = 0;  
	new_threadpool->queue_length = 0;

		new_threadpool->threads = (pthread_t*)malloc(sizeof(pthread_t)*num_threads);

		new_threadpool->queue = (pool_task_t*)malloc(sizeof(pool_task_t)*queue_size);


		if (pthread_cond_init(&(new_threadpool->notify), NULL) != 0 || pthread_mutex_init(&(new_threadpool->lock), NULL) != 0  || new_threadpool->threads == NULL || new_threadpool->queue == NULL) {
			pool_destroy(new_threadpool);
			return NULL;
		}
	int i;

		for (i=0; i < num_threads; i++) {
			if (pthread_create(&(new_threadpool->threads[i]), NULL, thread_do_work, (void*) new_threadpool) != 0) {
				pool_destroy(new_threadpool);
				return NULL;
			}
		}

	
		return new_threadpool;
}


/*
 * Add a task to the threadpool
 *
 */
int pool_add_task(pool_t *pool, void (*function)(void*), void *argument) {
	
	int err = 0;
	
	//lock the threadpool
	pthread_mutex_lock(&(pool->lock));

	

	//add it to the queue, even if it is empty


	if (enqueue(pool, function, argument) == 0){
	
		
		return 1; 
	}

	pool->active_threads++;

	// Broadcast notify

	
	pthread_cond_signal(&(pool->notify));
	
	//unlock the threadpool

	
	pthread_mutex_unlock(&(pool->lock));


	
	return err;
}

/*
 * Destroy the threadpool, free all memory, destroy treads, etc
 *
 */
int pool_destroy(pool_t *pool)
{
	int err = 0;


	

	pthread_mutex_destroy(&(pool->lock));
	pthread_cond_destroy(&(pool->notify));
	free(pool->threads);
	free(pool->queue);
	free(pool);
	
	return err;
}



/*
 * Work loop for threads. Should be passed into the pthread_create() method.
 *
 */

int threadID = 0;

static void *thread_do_work(void *pool)
{   
	pool_t* threadpool = (pool_t*)pool;
	int currThread = threadID;
	threadID++;
	while(1) {
		pthread_mutex_lock(&(threadpool->lock));
		while((threadpool->active_threads == 0) &&(threadpool->shutdown == 0)){
			pthread_cond_wait(&(threadpool->notify), &(threadpool->lock));
		}
		pool_task_t task = dequeue(threadpool);
		pthread_mutex_unlock(&(threadpool->lock));
		if(task.argument != NULL){
			threadpool->active_threads--;
			(*task.function)(task.argument);	
		}
	}
	pthread_exit(NULL);
	return(NULL);
}




