#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int ft_strncmp(int start, int len, char *line, char *test)
{
    int i = 0;

    while (line[start] && test[i] && line[start] == test[i] && i < len)
    {
        start++;
        i++;
    }
    if (i == len)
        return (0);
    return (line[start] - test[i]);
}

void    filter(char *line, char *tester)
{
    int i = 0;


    while (line[i])
    {
        if (ft_strncmp(i , strlen(tester), line, tester) == 0)
        {
            size_t j = 0;
            while (j < strlen(tester))
            {
                write(1, "*", 1);
                j++;
            }
            i += j;
        }
        else
        {
            write (1, &line[i], 1);
            i++;
        }
    }
}

int main(int ac, char **av)
{
    int count = 0;
    char *line;

    if (ac !=2)
        return (0);
    line = malloc(sizeof (char) * 1024);
    if (!line)
    {
        perror("Error: ");
        return (1);
    }
    count = read(0, line, 1024);
    if (count <= 0)
    {
        perror ("Error: ");
        return (1);
    }
    filter(line, av[1]);
}