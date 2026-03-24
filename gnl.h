#ifndef GNL
# define GNL

# ifndef BUFFER_SIZE
#  define BUFFER_SIZE 1024
# endif

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>


char	*get_next_line(int fd);

#endif