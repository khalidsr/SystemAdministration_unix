#include "../include/ft_nmap.h"

void ft_free(char **ptr)
{
    int i;
    i = 0;
    while (ptr[i])
        free(ptr[i++]);
    free(ptr);
}

int ft_len(char const *s, char c)
{
    int len;
    len = 0;
    while (*s)
    {
        if ((*s != c && len == 0) || (*s != c && *(s - 1) == c))
            len++;
        s++;
    }
    return (len);
}

char *ft_substr(const char *s, unsigned int start, size_t len)
{
    size_t  i;
    char   *ptr;

    i   = 0;
    ptr = NULL;
    if (!s)
        return (NULL);
    if (start > strlen(s))
        return (ft_strdup(""));
    if (len > strlen(s))
        len = strlen(s);
    ptr = malloc(sizeof(char) * (len + 1));
    if (!ptr)
        return (ptr);
    while (s[start] && i < len && start <= strlen(s))
        ptr[i++] = s[start++];
    ptr[i] = '\0';
    return (ptr);
}

char *ft_strdup(const char *s1)
{
    char *ptr;
    int   i;

    i   = 0;
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

char **ft_split(char const *s, char c)
{
    char **ptr;
    int    end;
    int    index;
    int    start;

    end   = 0;
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

bool is_valid_ip(const char *ip)
{
    int   num, dots = 0;
    char *ptr;
    char  ip_copy[20];

    if (!ip || strlen(ip) > 15)
        return false;

    strcpy(ip_copy, ip);
    ptr = strtok(ip_copy, ".");

    while (ptr) {
        if (!isdigit(ptr[0])) return false;
        num = atoi(ptr);
        if (num < 0 || num > 255) return false;
        ptr = strtok(NULL, ".");
        dots++;
    }
    return (dots == 4);
}

bool is_valid_ports(const char *ports, int *port_list, int *port_count)
{
    if (!ports)
        return false;

    char **port_specs = ft_split(ports, ',');
    if (!port_specs)
        return false;

    *port_count = 0;

    for (int i = 0; port_specs[i]; i++)
    {
        char *spec      = port_specs[i];
        char *range_sep = strchr(spec, '-');

        if (range_sep)
        {
            if (strchr(range_sep + 1, '-'))
            {
                ft_free(port_specs);
                return false;
            }
            *range_sep  = '\0';
            int start   = atoi(spec);
            int end     = atoi(range_sep + 1);

            if (start < 1 || end > MAX_PORTS || start > end)
            {
                ft_free(port_specs);
                return false;
            }
            for (int j = start; j <= end; j++)
            {
                if (*port_count >= MAX_PORTS)
                {
                    ft_free(port_specs);
                    return false;
                }
                port_list[(*port_count)++] = j;
            }
        }
        else
        {
            int num = atoi(spec);
            // if (num < 1 || num > MAX_PORTS)
            // {
            //     ft_free(port_specs);
            //     return false;
            // }
            port_list[(*port_count)++] = num;
        }
    }

    ft_free(port_specs);
    return true;
}

void print_error(char *str)
{
    printf("%s\n",str);
    exit(1);
}