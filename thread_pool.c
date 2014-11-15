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
  int shutdown;
};

static void *thread_do_work(void *pool);

/*
 * Add a task to the worker queue
 *
 */

int enqueue(pool_t *pool, void (*function)(void *), void *argument) {
  int i = 0;
	printf("starting add to queue\n");
	fflush(stdout);

  pool_task_t* newTask = malloc(sizeof(pool_task_t));
  newTask->argument = argument;
  newTask->function = function;
  // Pointers are being really weird here, but this compiles
  while (i < pool->task_queue_size_limit && pool->queue[i].argument != NULL) {
    i++;
  }
  if (i != pool->task_queue_size_limit) {
    pool->queue[i] = *(newTask);
	printf("enqueueing at: %x\n", &(pool->queue[i]));
	fflush(stdout);
  } else {
	printf("enqueue failed\n");
    return 0;
  }
	printf("finished enqueue\n");
  return 1;
}

/*
 * Remove task from worker queue
 *
 */

pool_task_t dequeue(pool_t *pool) {
  pool_task_t ret = pool->queue[0];
	printf("dequeueing, ret is: %x\n", ret);
	printf("ret.argument: %x, ret.function: %x\n", ret.argument, ret.function);
  int i = 1;
  while (pool->queue[i].argument != NULL) {
    pool->queue[i-1] = pool->queue[i];
    i++;
  }
  pool->queue[i-1].argument = NULL;
  pool->queue[i-1].function = NULL;

  return ret;
}


/*
 * Create a threadpool, initialize variables, etc
 *
 */
pool_t *pool_create(int queue_size, int num_threads)
{
	printf("creating pool...\n");
    pool_t* new_threadpool = (pool_t*)malloc(sizeof(pool_t));

    new_threadpool->task_queue_size_limit = queue_size;
    new_threadpool->thread_count = num_threads;
    new_threadpool->active_threads = 0;
    new_threadpool->shutdown = 0;
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
	printf("returning threadpool\n");
	fflush(stdout);
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
	printf("pool locked, adding task...\n");
	fflush(stdout);

	//add it to the queue, even if it is empty
	printf("function: %x, argument: %x\n", function, argument);

	if (enqueue(pool, function, argument) == 0){
		printf("queue is full\n");
		fflush(stdout);		
		return 1;	
	}

	pool->active_threads++;

	// Broadcast notify
	printf("sending signal...\n");
	fflush(stdout);
	pthread_cond_signal(&(pool->notify));
 	
	//unlock the threadpool
	printf("unlocking threadpool...\n");
	fflush(stdout);
	pthread_mutex_unlock(&(pool->lock));

	printf("returning %d...\n", err);
	fflush(stdout);
	return err;
}

/*
 * Destroy the threadpool, free all memory, destroy treads, etc
 *
 */
int pool_destroy(pool_t *pool)
{
    int err = 0;
    //int i = 0;

    // for (i=0; i < pool->thread_count; i++) {
    //   free(pool->threads[i]);
    // }
    // for (i=0; i < pool->task_queue_size_limit; i++) {
    //   free(pool->queue[i]);
    // }

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
		
		printf("waiting on thread %d\n", currThread);
		fflush(stdout);
		while((threadpool->active_threads == 0) &&(threadpool->shutdown == 0)){
			pthread_cond_wait(&(threadpool->notify), &(threadpool->lock));
		}

		printf("finished waiting on thread %d\n", currThread);
		fflush(stdout);

		pool_task_t task = dequeue(threadpool);

		printf("task->argument: %x, task->function: %x\n", task.argument, task.function);
		fflush(stdout);

		pthread_mutex_unlock(&(threadpool->lock));

		printf("got task, unlocked thread %d\n", currThread); 
		fflush(stdout);
		
      		if(task.argument != NULL){
			threadpool->active_threads--;
			printf("actual task, executing thread %d\n", currThread);
			fflush(stdout);

			(*task.function)(task.argument);

			printf("finished running task\n");
			fflush(stdout);
		}
		else{
			printf("task.argument is NULL\n"); 
			fflush(stdout);		
		}

	}

	printf("exiting thread %d\n", currThread);

	pthread_exit(NULL);
	return(NULL);
}



/*

pthread_create()
  event loop as start routine
  pulls from queue, runs that function, waits on condition variable to fiinish

create thread pool
  structure:
    worker queue with pending tasks
    each threads running in loop

    check for job in queue->if job, pull and run -> else wait on condition ->broadcast notify ->check for job...

  when job is added, pthread_cond_signal (&(pool->notify))

  ---

  add locking to seats.c
  standylist
    - semaphore
    */




