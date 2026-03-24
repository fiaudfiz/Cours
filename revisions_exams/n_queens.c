#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int ft_abs(int n)
{
    if(n < 0)
        return (n * -1);
    return (n);
}

int checker(int *tab, int colonne)
{
    int i = 0;
    while (i < colonne)
    {
        if (tab[i] == tab[colonne])
            return (1);
        if (ft_abs(tab[i] - tab[colonne]) == ft_abs(i - colonne))
            return (1);
        i++;
    }
    return (0);

}

void    resolve_n_queens(int queens, int *tab, int colonne)
{
    int ligne = 0;

    if (colonne == queens)
    {
        int i = 0;
        while (i < queens)
        {
            if (i + 1 == queens)
            {
                printf("%d", tab[i]);
            }
            else
                printf("%d ", tab[i]);
            i++;
        }
        printf("\n");
        return ;
    }
    while (ligne < queens)
    {
        tab[colonne] = ligne;
        if (checker(tab, colonne) == 0)
        {
            resolve_n_queens(queens, tab, colonne + 1);
        }
        ligne++;
    }
    if (ligne == queens)
        return ;
}

int main(int ac, char **av)
{
    int *tab;
    int taille;

    if (ac != 2)
        return (0);
    taille = atoi(av[1]);
    tab = malloc (sizeof (int) * taille + 1);
    if (!tab)
        return (0);
    resolve_n_queens(taille, tab, 0);
}