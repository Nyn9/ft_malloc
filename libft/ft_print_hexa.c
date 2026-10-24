#include "libft.h"

int	ft_print_hexa(unsigned int nb, char *base)
{
	int		len;

	len = 0;
	if (nb < 16)
		len += ft_printchar(base[nb]);
	else
	{
		len += ft_print_hexa(nb / 16, base);
		len += ft_printchar(base[nb % 16]);
	}
	return (len);
}
