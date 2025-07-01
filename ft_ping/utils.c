#include "ft_ping.h"

void print_help()
{
    printf("Usage: ft_ping [options] destination\n");
    printf("Options:\n");
    printf("  -c count       Stop after sending count packets\n");
    printf("  -i interval    Wait interval seconds between sending each packet\n");
    printf("  -n             Numeric output only (no DNS resolution)\n");
    printf("  -q             Quiet output (only summary)\n");
    printf("  -s size        Specify packet size\n");
    printf("  -v             Verbose output\n");
    printf("  -?  or -h           Show this help message\n");
}

void ft_free(char **ptr)
{
    int i;
    i = 0;
    while (ptr[i])
	{
		free(ptr[i]);
		i++;
	}
        
    free(ptr);
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

char	*ft_substr(const char *s, unsigned int start, size_t len)
{
	size_t	i;
	char	*ptr;

	i = 0;
	ptr = NULL;
	if (!s)
		return (NULL);
	if (start > strlen(s))
	{
		return (ft_strdup(""));
	}
	if (len > strlen(s))
	{
		len = strlen(s);
	}
	ptr = malloc(sizeof(char) * (len + 1));
	if (!ptr)
		return (ptr);
	while (s[start] && i < len && start <= strlen(s))
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
	ptr = malloc((strlen(s1) + 1) * sizeof(char));
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