#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

void    *function_first_thread(void *arg)
{
    printf("first thread\n");
    //arret du thread
    pthread_exit(EXIT_SUCCESS);
}

int     main(void)
{
    //creation de la variable qui va contenir le thread
    pthread_t   thread_1;
    printf("avant la decla\n");
    pthread_create(&thread_1, NULL, function_first_thread, NULL);
    pthread_join(thread_1, NULL);
    printf ("Arpres la decla\n");
    return (EXIT_SUCCESS);
}