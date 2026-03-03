#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Nombre total de thread
#define NB_THREAD 2
// Limite de l'incrément
#define INCREMENT_LIMIT 1000000

// Tableau contenant les threads
pthread_t threads[NB_THREAD];

typedef struct mutex_data {
	int data;
	pthread_mutex_t mutex;
} mutex_data;

void    *job1(void *arg) // fonction executee par le thread1
{
    mutex_data  *md = (mutex_data *)arg;
    pthread_t   tid = pthread_self();
    int         i;
    
    i = 0;
    while (i < 15)
    {
        pthread_mutex_lock(&(*md).mutex);
        (*md).data++;
        usleep(300000);
        pthread_mutex_unlock(&(*md).mutex);
        usleep(100000);
        i++;
       
    }
    pthread_exit(NULL);
}


void    *job2(void *arg) // fonction executee par le thread2
{
    mutex_data  *md = (mutex_data *)arg;
    pthread_t   tid = pthread_self();
    int         i;
    
    i = 0;
    while (i < 30)
    {
        if (pthread_mutex_trylock(&(*md).mutex) == 0)
        {
            printf ("Valeur du compteur : %d\n", (*md).data);
            pthread_mutex_unlock(&(*md).mutex);
            usleep(200000);
        }
        else
        {
            printf ("mutex occupe, je vais aller dormir\n");
            usleep(150000);
        }
        i++;
    }
    pthread_exit(NULL);
}

int main(void)
{
    mutex_data    md;
    int           i;
    int           error_thread;

    error_thread = 0;
    i = 0;
    md.data = 0;
    if (pthread_mutex_init(&md.mutex, NULL) != 0) 
    {
		printf("\n mutex init failed\n");
		return (EXIT_FAILURE);
	}
    if (pthread_create(&threads[0], NULL, job1, &md) != 0)
    {
        printf("error in thread\n");
        return (EXIT_FAILURE);
    }
    printf("creation du thread numero %ld\n", threads[0]);
    if (pthread_create(&threads[1], NULL, job2, &md) != 0)
    {
        printf("error in thread\n");
        return (EXIT_FAILURE);
    }
    printf("creation du thread numero %ld\n", threads[1]);
    //on attend les threads
    i = 0;
    while (i < NB_THREAD)
    {
        pthread_join(threads[i], NULL);
        i++;
    }
	pthread_mutex_destroy(&md.mutex);
	return EXIT_SUCCESS;
}