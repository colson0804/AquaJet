#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include "seats.h"
#include "semaphore.h"






void sem_init(m_sem_t *s, int value) {
	s->count = value;
	pthread_mutex_t mutex;
	pthread_cond_t condv;
	pthread_cond_init(&condv, NULL);
	pthread_mutex_init(&mutex, NULL);
	s->condv = condv;
	s->mutex = mutex;
}

int sem_wait(m_sem_t *s)
{
    pthread_mutex_lock(&(s->mutex));
	printf("at semwait, scount: %d\n", s->count);
	fflush(stdout);
    while (s->count <= 0) {
    	pthread_cond_wait(&(s->condv), &(s->mutex));
    }
	printf("donewaiting\n");
	fflush(stdout);
    s->count--;
    pthread_mutex_unlock(&(s->mutex));
    return 0;
}

int sem_post(m_sem_t *s)
{
    pthread_mutex_lock(&(s->mutex));
    s->count++;
    pthread_cond_broadcast(&(s->condv));
    pthread_mutex_unlock(&(s->mutex));
    return 0;
}
