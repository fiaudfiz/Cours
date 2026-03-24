#include <stdio.h>
#include <stdlib.h>


void    power_set(int cible, int *tab, int index_of_depart, int taille, int *result, int index_of_depart_result, int taille_result)
{
    if(index_of_depart == taille)   
    {
        if (cible == 0)
        {
            int i = 0;
            while (i < taille_result)
            {
                
                if (i + 1 == taille_result)
                {
                    printf("%d", result[i]);
                }
                else
                    printf("%d ", result[i]);
                i++;
            }
            printf("\n");
            return ;
        }
        else
            return ;
    }
    result[index_of_depart_result] = tab[index_of_depart];
    power_set(cible - tab[index_of_depart], tab, index_of_depart + 1, taille, result, index_of_depart_result + 1, taille_result + 1);
    power_set(cible, tab, index_of_depart + 1, taille, result, index_of_depart_result, taille_result);
    
    
}






int main(int ac, char **av)
{
    int *tab;
    int i = 1;
    int taille = 0;
    int *result;

    if (ac <= 2)
        return (0);
    tab = malloc (sizeof (int) * ac);
    if (!tab)
        return (0);
    while (av[i])
    {
        tab[taille] = atoi(av[i]);
        i++;
        taille++;
    }
    result = malloc (sizeof (int) * taille + 1);
    if (!result)
        return (0);
    power_set(tab[0], tab, 0, taille, result, 0, 0);
}