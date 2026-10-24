#include "libft.h"

int	ft_printunbr(unsigned int n)
{
	int	len;

	len = 0;
	if (n < 10)
		len += ft_printchar(n + '0');
	else
	{
		len += ft_printunbr(n / 10);
		len += ft_printchar((n % 10) + '0');
	}
	return (len);
}
