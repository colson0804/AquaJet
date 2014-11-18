#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "seats.h"
#include "semaphore.h"

seat_t* seat_header = NULL;

char seat_state_to_char(seat_state_t);

pthread_mutex_t seatLock;

standby_t* standby = NULL;

void list_seats(char* buf, int bufsize)
{
	pthread_mutex_lock(&(seatLock));
  
    seat_t* curr = seat_header;
    int index = 0;
    while(curr != NULL && index < bufsize+ strlen("%d %c,"))
    {
        int length = snprintf(buf+index, bufsize-index, 
                "%d %c,", curr->id, seat_state_to_char(curr->state));
        if (length > 0)
            index = index + length;
        curr = curr->next;
    }
    if (index > 0)
        snprintf(buf+index-1, bufsize-index-1, "\n");
    else
        snprintf(buf, bufsize, "No seats not found\n\n");

	pthread_mutex_unlock(&(seatLock));
}

void view_seat(char* buf, int bufsize,  int seat_id, int customer_id, int customer_priority)
{
	printf("Customer %d viewing seat: %d\n",customer_id, seat_id);	
	pthread_mutex_lock(&(seatLock));
    seat_t* curr = seat_header;
    while(curr != NULL)
    {
        if(curr->id == seat_id)
        {
            if(curr->state == AVAILABLE || (curr->state == PENDING && curr->customer_id == customer_id))
            {
                snprintf(buf, bufsize, "Confirm seat: %d %c ?\n\n",
                        curr->id, seat_state_to_char(curr->state));
                curr->state = PENDING;
                curr->customer_id = customer_id;

            }
            else if (curr->state == PENDING)
            {
		printf("Searching standby list\n");                
		standby_t* temp = standby;
                while (temp != NULL) {
                    temp = temp->next;
                }
                temp = (standby_t*)malloc(sizeof(standby_t));
		if (standby == NULL){
			standby = temp;		
		}
                temp->currSeat = curr;
		temp->sem = (m_sem_t*)malloc(sizeof(m_sem_t));
                
		sem_init(temp->sem, 0);
                temp->next = NULL;
		printf("Current seat: %d\n", temp->currSeat->id);
		pthread_mutex_unlock(&(seatLock));

		printf("standby list is now: %x\n", standby);
		fflush(stdout);

                sem_wait(temp->sem);
		pthread_mutex_lock(&(seatLock));
		printf("Done waiting\n");
		curr->customer_id = customer_id;

		pthread_mutex_unlock(&(seatLock));
            }
            else
            {
                snprintf(buf, bufsize, "Seat unavailable\n\n");

            }
		
		pthread_mutex_unlock(&(seatLock));
            return;
        }
        curr = curr->next;
    }
    snprintf(buf, bufsize, "Requested seat not found\n\n");
	pthread_mutex_unlock(&(seatLock));
    return;
}

void confirm_seat(char* buf, int bufsize, int seat_id, int customer_id, int customer_priority)
{
	printf("custom %d is confirming seat %d\n",customer_id, seat_id);
    pthread_mutex_lock(&(seatLock));
    seat_t* curr = seat_header;
    while(curr != NULL)
    {       
	if(curr->id == seat_id)
        {
    
	if(curr->state == PENDING && curr->customer_id == customer_id )
            {
		printf("Current seat: %d\n", curr->id);                
		snprintf(buf, bufsize, "Seat confirmed: %d %c\n\n",
                        curr->id, seat_state_to_char(curr->state));
                curr->state = OCCUPIED;
                standby_t* temp = standby;
                while (temp != NULL && temp->next != NULL) {
                    if (temp->next->currSeat == curr) {
                        standby_t* badSeat = temp->next;
                        temp->next = temp->next->next;
			free(badSeat->sem);                        
			free(badSeat);
                    }
                }
            }
            else if(curr->customer_id != customer_id )
            {
                snprintf(buf, bufsize, "Permission denied - seat held by another user\n\n");
            }
            else if(curr->state != PENDING)
            {
                snprintf(buf, bufsize, "No pending request\n\n");
            }
	pthread_mutex_unlock(&(seatLock));

            return;
        }
        curr = curr->next;
    }
    snprintf(buf, bufsize, "Requested seat not found\n\n");
    	pthread_mutex_unlock(&(seatLock));
    return;
}

void cancel(char* buf, int bufsize, int seat_id, int customer_id, int customer_priority)
{
	printf("customer %d is canceling seat: %d\n", customer_id, seat_id);
	pthread_mutex_lock(&(seatLock));

    seat_t* curr = seat_header;
    while(curr != NULL)
    {

        if(curr->id == seat_id)
        {
		printf("seat_id: %d is the same\n", seat_id);
		fflush(stdout);
            if(curr->state == PENDING && curr->customer_id == customer_id )
            {
		printf("customer_id: %d is the same\n", customer_id);
		fflush(stdout);

                printf("Seat request cancelled: %d %c\n\n",
                        curr->id, seat_state_to_char(curr->state));
		fflush(stdout);
                curr->state = AVAILABLE;

                standby_t* temp = standby;
		printf("Standby is: %x\n", standby);
		fflush(stdout);
		
		if (temp != NULL && temp->currSeat == curr){
			printf("SEMPOSTING\n");
			fflush(stdout);
                        standby_t* badSeat = temp;
                        sem_post(badSeat->sem);
                        free(badSeat);
			temp = NULL;
			pthread_mutex_unlock(&(seatLock));
			return;
		}
		else{                
			while (temp != NULL && temp->next != NULL) {
				if (temp->next->currSeat == curr) {
					printf("SEMPOSTING\n");
					fflush(stdout);
                        		standby_t* badSeat = temp->next;
                        		temp->next = temp->next->next;
                       			sem_post(badSeat->sem);

                        		free(badSeat);
					pthread_mutex_unlock(&(seatLock));
					return;
                    		}
                	}
		}



            }
            else if(curr->customer_id != customer_id )
            {
                snprintf(buf, bufsize, "Permission denied - seat held by another user\n\n");
            }
            else if(curr->state != PENDING)
            {
                snprintf(buf, bufsize, "No pending request\n\n");
            }
		pthread_mutex_unlock(&(seatLock));
            return;
        }
        curr = curr->next;
    }
    snprintf(buf, bufsize, "Seat not found\n\n");
    	pthread_mutex_unlock(&(seatLock));
    return;
}

void load_seats(int number_of_seats)
{
	pthread_mutex_init(&(seatLock), NULL);	

	pthread_mutex_lock(&(seatLock));

    seat_t* curr = NULL;
    int i;
    for(i = 0; i < number_of_seats; i++)
    {   
        seat_t* temp = (seat_t*) malloc(sizeof(seat_t));
        temp->id = i;
        temp->customer_id = -1;
        temp->state = AVAILABLE;
        temp->next = NULL;
        
        if (seat_header == NULL)
        {
            seat_header = temp;
        }
        else
        {
            curr-> next = temp;
        }
        curr = temp;
    }
	pthread_mutex_unlock(&(seatLock));
}

void unload_seats()
{
	pthread_mutex_destroy(&(seatLock));
    seat_t* curr = seat_header;
    while(curr != NULL)
    {
        seat_t* temp = curr;
        curr = curr->next;
        free(temp);
    }
}

char seat_state_to_char(seat_state_t state)
{
    switch(state)
    {
        case AVAILABLE:
            return 'A';
        case PENDING:
            return 'P';
        case OCCUPIED:
            return 'O';
    }

    return '?';
}
