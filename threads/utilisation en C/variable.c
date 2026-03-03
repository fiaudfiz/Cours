#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

void    *function_first_thread(void *arg)
{
    int *i  = (int *)arg;
    (*i)++;
    //arret du thread
    pthread_exit(NULL);
}

int     main(void)
{
    //creation de la variable qui va contenir le thread
    pthread_t   thread_1;
    int i;

    i = 1;
    printf("avant la decla, i = %d\n", i);
    pthread_create(&thread_1, NULL, function_first_thread, &i);
    pthread_join(thread_1, NULL);
    printf ("Arpres la decla, i = %d\n", i);
    return (EXIT_SUCCESS);
}