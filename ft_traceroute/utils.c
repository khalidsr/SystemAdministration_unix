#include "ft_traceroute.h"

void print_help()
{
    printf("Usage: ft_traceroute [OPTIONS] HOSTNAME/IP\n");
    printf("Options:\n  --help     Show this help message\n");
	printf("  -n               Do not resolve hostnames (IP only)\n");
	printf("  -f <first_ttl>   Set the first TTL to start with\n");
	printf("  -m <max_ttl>     Set the max number of hops (default: 30)\n");
}


void ft_free(char **ptr)
{
    int i;
    i = 0;
    while (ptr[i])
        free(ptr[i++]);
    free(ptr);
}
int	ft_isdigit(int c)
{
	if (!(c >= '0' && c <= '9'))
		return (0);
	return (1);
}

int	ft_strcmp(const char *s1, const char *s2)
{
	size_t			i;
	unsigned char	*ss1;
	unsigned char	*ss2;

	i = 0;
	ss1 = (unsigned char *)s1;
	ss2 = (unsigned char *)s2;
	while ((ss1[i] || ss2[i]))
	{
		if (ss1[i] != ss2[i])
			return (ss1[i] - ss2[i]);
		i++;
	}
	return (0);
}

int	ft_len(char const *s, char c)
{
	int	len;

	len = 0;
	while (*s)
	{
		if ((*s != c && len == 0) || (*s != c && *(s - 1) == c))
			len++;
		s++;
	}
	return (len);
}


size_t ft_strlen(const char *str)
{
	int i;

	i = 0;
	while (str[i])
		i++;
	return i;
}
char	*ft_substr(const char *s, unsigned int start, size_t len)
{
	size_t	i;
	char	*ptr;

	i = 0;
	ptr = NULL;
	if (!s)
		return (NULL);
	if (start > ft_strlen(s))
	{
		return (ft_strdup(""));
	}
	if (len > ft_strlen(s))
	{
		len = ft_strlen(s);
	}
	ptr = malloc(sizeof(char) * (len + 1));
	if (!ptr)
		return (ptr);
	while (s[start] && i < len && start <= ft_strlen(s))
	{
		ptr[i++] = s[start++];
	}
	ptr[i] = '\0';
	return (ptr);
}

char	*ft_strdup(const char *s1)
{
	char	*ptr;
	int		i;

	i = 0;
	ptr = malloc((ft_strlen(s1) + 1) * sizeof(char));
	if (!ptr)
		return (NULL);
	while (s1[i])
	{
		ptr[i] = s1[i];
		i++;
	}
	ptr[i] = '\0';
	return (ptr);
}


char	**ft_split(char const *s, char c)
{
	char	**ptr;
	int		end;
	int		index;
	int		start;

	end = 0;
	start = 0;
	index = 0;
	if (!s)
		return (NULL);
	ptr = malloc(sizeof(char *) * (ft_len(s, c) + 1));
	if (!ptr)
		return (NULL);
	while (index < ft_len(s, c))
	{
		while (s[end] == c && s[end])
			end++;
		start = end;
		while (s[end] != c && s[end])
			end++;
		ptr[index] = ft_substr(s, start, end - start);
		index++;
	}
	ptr[index] = NULL;
	return (ptr);
}
int ft_atoi(const char *str)
{
	int i;
	int res;
	int sign;
	i = 0;
	res = 0;
	sign = 1;

	while((str[i] >= 9 && str[i]<= 13 )|| str[i] == ' ')
		i++;
	if(str[i] == '-')
		sign *= (-1);
	else
		sign *= 1;
	i++;
	while(str[i]>= '0' && str[i]<= '9')
	{
		res = res *10 + str[i] - '0';
		i++;
	}
	return (res* sign);
}



void	*ft_memset(void *b, int c, size_t len)
{
	unsigned char	*ptr;
	size_t			i;

	i = 0;
	ptr = (unsigned char *) b;
	while (i < len)
	{
		ptr[i] = (unsigned char) c;
		i++;
	}
	return (ptr);
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