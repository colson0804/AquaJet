#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "seats.h"
#include "semaphore.h"



void sem_init(m_sem_t *s, int value) {
	s->count = value;
	if (pthread_cond_init(&(s->condv), NULL) != 0 || pthread_mutex_init(&(s->mutex), NULL) != 0) {
		printf("Not working...\n");
		return;
	}

}

int sem_wait(m_sem_t *s)
{
    pthread_mutex_lock(&(s->mutex));
    while (s->count <= 0) {
    	pthread_cond_wait(&(s->condv), &(s->mutex));
    }
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
