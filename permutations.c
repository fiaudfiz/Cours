#include <stdio.h>
#include <stdlib.h>

int ft_strlen(char *str)
{
    int i = 0;

    while (str[i])
    {
        i++;
    }
    return(i);
}

void    swap(char *str, int i, int j)
{
    char temp;

    temp = str[i];
    str[i] = str[j];
    str[j] = temp;
}


char *sort_str(char *str)
{
    int i = 0;
    int j = i + 1;

    while (str[i] && str[j])
    {
        j = i + 1;
        while (str[j])
        {
            if (str[j] < str[i])
            {
                swap(str, i, j);
                i = 0;
            }
            else
                j++;
        }
        i++;
    }
    return (str);
}

void    permutations(char *str, int index_of_depart, int taille)
{
    if (index_of_depart == taille)
    {
        printf("%s\n", str);
        return;
    }
    int i = index_of_depart;
    while (i < taille)
    {
        swap(str, index_of_depart, i);
        permutations(str, index_of_depart + 1, taille);
        swap (str, index_of_depart, i);
        i++;
    }
}

int main(int ac, char **av)
{
    char *str_sorted;

    if (ac != 2)
        return (0);
    str_sorted = sort_str(av[1]);
    permutations(str_sorted, 0, ft_strlen(str_sorted));

}