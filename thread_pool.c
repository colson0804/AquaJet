#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

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

typedef struct {
    void (*function)(void *);
    void *argument;
} pool_task_t;


struct pool_t {
  pthread_mutex_t lock;
  pthread_cond_t notify;
  pthread_t *threads;
  pool_task_t *queue;
  int thread_count;
  int task_queue_size_limit;
};

static void *thread_do_work(void *pool);


/*
 * Create a threadpool, initialize variables, etc
 *
 */
pool_t *pool_create(int queue_size, int num_threads)
{
    pool_t* new_threadpool = malloc(sizeof(pool_t));

    new_threadpool->task_queue_size_limit = queue_size;
    new_threadpool->thread_count = num_threads;

    new_threadpool->threads = malloc(sizeof(pthread_t)*num_threads);

    new_threadpool->queue = malloc(sizeof(pool_task_t)*queue_size);

    return new_threadpool;
}


/*
 * Add a task to the threadpool
 *
 */
int pool_add_task(pool_t *pool, void (*function)(void*), void *argument) {
    int err = 0;
    pthread_t* new_thread = malloc(sizeof(pthread_t));

    int i = 0;
    while((i != pool->thread_count) && (pool->threads[i] != NULL)){
      i++;
    }

    if (i != pool->thread_count){
      err = pthread_create(&(pool->threads[i]), NULL, &thread_do_work, argument);   
    }
    else{
      i = 0;
      while((i != pool->task_queue_size_limit) && (pool->queue[i] != NULL)){  //THIS ISNT WORKING
        i++;
      }
      if (i == pool->task_queue_size_limit){
        return 1;
      }
      else{
        pool_task_t* new_task;
        new_task->function = function;
        new_task->argument = argument;

        pool->queue[i] = *new_task;
      }
    }

    return err;
}

/*
 * Destroy the threadpool, free all memory, destroy treads, etc
 *
 */
int pool_destroy(pool_t *pool)
{
    int err = 0;

    free(pool);
 
    return err;
}



/*
 * Work loop for threads. Should be passed into the pthread_create() method.
 *
 */
static void *thread_do_work(void *pool)
{ 

    while(1) {

      /*
        while((pool->count == 0) && (!pool->shutdown)){
          pthread_cond _wait(&pool->notify), &(pool->lock);
        }
        something about 'someone may be there already'

      */
        
    }

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




