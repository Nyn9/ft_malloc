#include "libft.h"

char	*ft_strjoin(char const *s1, char const *s2)
{
	char	*new;
	size_t	i;
	size_t	j;
	size_t	lens1;
	size_t	lens2;

	i = 0;
	j = 0;
	lens1 = ft_strlen(s1);
	lens2 = ft_strlen(s2);
	new = malloc(sizeof(char) * (lens1 + lens2 + 1));
	if (!new)
		return (NULL);
	while (i < lens1)
	{
		new[i] = s1[i];
		i++;
	}
	while (j < lens2)
		new[i++] = s2[j++];
	new[i] = '\0';
	return (new);
}
