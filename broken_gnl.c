/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   broken_gnl.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erazumov <erazumov@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/11 16:56:00 by erazumov          #+#    #+#             */
/*   Updated: 2025/07/18 15:42:59 by erazumov         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "get_next_line.h"

char	*ft_strchr(char *s, int c)
{
	int	i = 0;
    //printf("strchr\n");
	while (s[i] != c && s[i])//ajout
		i++;
	if (s[i] == c)
    {
        //printf("ici\n");
		return (&s[i]);
    }
	else
		return (NULL);
}

void *ft_memcpy(void *dest, const void *src, int n)//fonction changee
{
    int i = 0;
    while (i < n)
    {
        ((char *)dest)[i] = ((char *)src)[i];
        i++;
    }
    return (dest);
}   

size_t	ft_strlen(char *s)
{
    size_t	ret = 0;

    if (!s)
    {
        return (0);
    }
	while (s[ret])
	{
		ret++;
	}
	return (ret);
}
void    ft_wesh(char *dest, int index_of_depart, char *src, size_t taille_src)
{
    size_t i = 0;

    while (i < taille_src)
    {
        dest[index_of_depart] = src[i];
        i++;
        index_of_depart++;
    }
    return ;
}


int	str_append_mem(char **s1, char *s2, size_t size2)
{
	size_t	size1 = ft_strlen(*s1);
	char	*tmp = malloc(size2 + size1 + 1);
	if (!tmp)
		return (0);
	ft_memcpy(tmp, *s1, (int)size1);
    //printf("good\n");
	ft_wesh(tmp, size1, s2, (int)size2);
    //printf("?\n");
	tmp [size1 + size2] = '\0';     
	*s1 = tmp;
	return (1);
}

int	str_append_str(char **s1, char *s2)
{
	return (str_append_mem(s1, s2, ft_strlen(s2)));
}

void	*ft_memmove(void *dest, const void *src, size_t n)
{
	if (dest > src)
		return (ft_memcpy(dest, src, n));
	else if (dest == src)
		return (dest);
	int i = 0;
    while (i < (int)n)
    {
        ((char *)dest)[i] = ((char *)src)[i];
        i++;
    }
	return (dest);
}

char	*get_next_line(int fd)
{
	static char	b[BUFFER_SIZE + 1];
	char	*ret = NULL;

	char	*tmp = ft_strchr(b, '\n');
    //printf("hello\n");
	while (!tmp)
	{
        //printf("tmp vide\n");
		if (!str_append_str(&ret, b))
        {   
            //printf("a");
			return (NULL);
        }
		int	read_ret = read(fd, b, BUFFER_SIZE);
		if (read_ret == -1)
        {
            //printf("read = -1\n");
			return (NULL);
        }
        if (read_ret == 0)
            break ;
		b[read_ret] = '\0';
        tmp = ft_strchr(b, '\n');
	}
   
	if (!str_append_mem(&ret, b, tmp - b + 1))
	{
        //printf("c\n");
		free(ret);
		return (NULL);
	}
     ft_memmove(b, tmp + 1, ft_strlen(tmp + 1) + 1);
	return (ret);
}