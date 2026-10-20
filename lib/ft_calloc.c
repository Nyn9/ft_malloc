#include "libft.h"

void	*ft_calloc(size_t nmemb, size_t size)
{
	void	*new;
	size_t	i;

	i = 0;
	if (nmemb == 0 || size == 0)
		new = malloc(0);
	else
		new = malloc(nmemb * size);
	if (!new)
		return (NULL);
	while (*(unsigned char *)(new + i))
	{
		*(unsigned char *)(new + i) = 0;
		i++;
	}
	return (new);
}
