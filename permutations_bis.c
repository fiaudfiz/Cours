#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int ft_strlen(char *s)
{
    int i = 0;

    while (s[i])
    {
        i++;
    }
    return (i);
}

void swap(char *str, int i, int j)
{
    char temp;

    temp = str[i];
    str[i] = str[j];
    str[j] = temp;

}

void    permutations(char *str, int taille, int index_of_depart)
{
    if (index_of_depart == taille)
    {
        int i = 0;
        while (i < taille)
        {
            write (1, &str[i], 1);
            i++;
        }
        write (1, "\n", 1);
    }
    int j = index_of_depart;
    while (j < taille)
    {
        swap(str, j, index_of_depart);
        permutations(str, taille, index_of_depart + 1);
        swap(str, j, index_of_depart);
        j++;
    }
}


int main(int ac, char **av)
{
    char *str;
    int taille = ft_strlen(av[1]);
    int index_of_depart = 0;
    int i = 0;
    if (ac != 2)
        return (0);
    str = malloc (sizeof(char) * ft_strlen(av[1]));
    if (!str)
        return (0);
    while (i < taille)
    {
        str[i] = av[1][i];
        i++;
    }
    str[taille] = '\0';
    permutations(str, taille, index_of_depart);
}