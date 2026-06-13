#include "malloc.h"

int ft_strcmp(const char *s1, const char *s2)
{
    size_t i;
	
    i = 0;
    while (s1[i] == s2[i] && s1[i] != '\0')
	i++;
return ((unsigned char)s1[i] - (unsigned char)s2[i]);
}

size_t ft_strlen(char* str)
{
	size_t i=0;

	while(str[i])
		i++;
	return i;
}

void	*ft_memcpy(void *dest, const void *src, size_t n)
{
	size_t	i;
	char	*ddest;
	char	*ssrc;

	i = 0;
	ddest = (char *)dest;
	ssrc = (char *)src;
	if (!ddest && !ssrc)
		return (NULL);
	while (i < n)
	{
		ddest[i] = ssrc[i];
		i++;
	}
	return (ddest);
}

static int ft_putchar(char c)
{
    write(1, &c, 1);
    return (1);
}

static int ft_putstr(const char *str)
{
    int count = 0;

    if (!str)
        str = "(null)";

    while (*str)
    {
        count += ft_putchar(*str);
        str++;
    }
    return (count);
}

static int ft_putsize_t(size_t n)
{
    int count = 0;

    if (n >= 10)
        count += ft_putsize_t(n / 10);

    count += ft_putchar((n % 10) + '0');
    return (count);
}

static int ft_putptr(unsigned long ptr)
{
    const char *hex = "0123456789abcdef";
    int count = 0;

    if (ptr >= 16)
        count += ft_putptr(ptr / 16);

    count += ft_putchar(hex[ptr % 16]);
    return (count);
}

static int ft_printaddress(void *ptr)
{
    if (!ptr)
        return (ft_putstr("(nil)"));

    return (ft_putstr("0x") + ft_putptr((unsigned long)ptr));
}

int ft_printf(const char *fmt, ...)
{
    va_list args;
    int i = 0;
    int count = 0;

    va_start(args, fmt);

    while (fmt[i])
    {
        if (fmt[i] == '%')
        {
            i++;

            if (fmt[i] == 's')
                count += ft_putstr(va_arg(args, char *));
            else if (fmt[i] == 'p')
                count += ft_printaddress(va_arg(args, void *));
            else if (fmt[i] == '%' )
                count += ft_putchar('%');
            else if (fmt[i] == 'z' && fmt[i + 1] == 'u')
            {
                count += ft_putsize_t(va_arg(args, size_t));
                i++;
            }
        }
        else
            count += ft_putchar(fmt[i]);

        i++;
    }

    va_end(args);
    return (count);
}