#ifndef _SEM_OPERATIONS_H_
#define _SEM_OPERATIONS_H_

typedef struct m_sem_t {
    pthread_mutex_t mutex;
 	pthread_cond_t condv;
 	int count;
 	seat_t seatid;
 	struct m_sem_t* next;
} m_sem_t;

typedef struct standby_t {
	seat_t* currSeat;
	m_sem_t* sem;
	struct standby_t* next;
};

int sem_wait(m_sem_t *s);
int sem_post(m_sem_t *s);
void sem_init(m_sem_t *s, int value);

#endif