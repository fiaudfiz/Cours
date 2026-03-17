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

void    permutations(char *str, int *used,char *result, int index_of_result, int taille)
{
    if (index_of_result == taille)
    {
        result[taille] = '\0';
        printf("%s\n", result);
        return;
    }
    int i = 0;
    while (i < taille)
    {
        if (!used[i])
        {
            used[i] = 1;
            result[index_of_result] = str[i];
            permutations(str, used, result, index_of_result + 1, taille);
            used[i] = 0;
        }
        i++;
    }
}

int main(int ac, char **av)
{
    char *str_sorted;
    char *result;
    int *used;
    int taille = ft_strlen(av[1]);

    if (ac != 2)
        return (0);
    str_sorted = sort_str(av[1]);
    result = malloc(sizeof(char) * (taille + 1));
    used = malloc(sizeof(int) * taille);
    if (!result || !used)
        return (1);
    int i = 0;
    while (i < taille)
    {
        used[i] = 0;
        i++;
    }
    permutations(str_sorted, used, result, 0, taille);

}