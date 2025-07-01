#include "ft_ping.h"
#include <ctype.h>
#include <limits.h>

int is_positive_integer(const char *str)
{
    if (!str || *str == '\0')
        return 0;
    if (*str == '0' && str[1] != '\0')
        return 0;

    for (int i = 0; str[i] != '\0'; i++)
    {
        if (!isdigit(str[i]))
            return 0;
    }

    long num = atol(str);
    if (num <= 0 || num > INT_MAX)
        return 0;

    return 1;
}

int check_flag(char *str)
{
    int i = 1;
    int len = strlen(str);
    if (len <= 1) 
        return 0;

    while (i < len)
    {
        if (str[i] == '?' || str[i] == 'h')
        {
            print_help();
            exit(0);
        }
        else if (str[i] != 'v' && str[i] != 'n' && str[i] != 'q')
        {
            printf("ft_ping: invalid option -- '%c'\n", str[i]);
            printf("Try 'ft_ping -?' for more information.\n");
            exit(1);
        }
        i++;
    }
    return 1;
}

int check_ip(char *str)
{
    int i = 0;
    char **ptr;
    int num;
    
    ptr = ft_split(str, '.');
    if (!ptr) 
        return 0;

    while (ptr[i])
        i++;
    if (i != 4)
    {
        ft_free(ptr);
        return 0;
    }
    
    i = 0;
    while (ptr[i])
    {
        if (ptr[i][0] == '\0')
        {
            ft_free(ptr);
            return 0;
        }
        
        int j = 0;
        while (ptr[i][j])
        {
            if (!isdigit(ptr[i][j]))
            {
                ft_free(ptr);
                return 0;
            }
            j++;
        }
        num = atoi(ptr[i]);
        if (num < 0 || num > 255)
        {
            ft_free(ptr);
            return 0;
        }
        
        if (ptr[i][0] == '0' && ptr[i][1] != '\0')
        {
            ft_free(ptr);
            return 0;
        }
        
        i++;
    }
    ft_free(ptr);

    return 1;
}

char *extract_non_numeric(const char *str)
{
    if (!str)
        return NULL;

    int i = 0;
    while (str[i] != '\0' && (isdigit(str[i]) || str[i] == '.'))
    {
        i++;
    }
    if (str[i] == '\0')
    {
        char *result = malloc(1);
        if (result)
            result[0] = '\0';
        return result;
    }

    int len = strlen(str + i);
    char *result = malloc(len + 1);
    if (!result)
        return NULL;

    strcpy(result, str + i);
    return result;
}

int is_valid_float(const char *str)
{
    int dot_count = 0;

    if (!str || *str == '\0')
        return 0;

    for (int i = 0; str[i] != '\0'; i++)
    {
        if (!isdigit(str[i]))
        {
            if (str[i] == '.' && dot_count == 0)
                dot_count++;
            else
                return 0;
        }
    }
    return 1; 
}

void pars(int ac, char **av, t_ping *ping)
{
    int i = 0;
    int k = 0;
    int t = 0;

    if (ac < 2)
    {
        printf("ping: usage error: Destination address required\n");
        exit(1);
    }
    ping->hostname = calloc(ac, sizeof(char *));
    ping->target = calloc(ac, sizeof(char *));
    
    if (!ping->target || !ping->hostname)
    {
        perror("calloc");
        exit(1);
    }

    for (i = 1; i < ac; i++)
    {
        if (strcmp(av[i], "-c") == 0 && i + 1 < ac)
        {
            if (is_positive_integer(av[i + 1]))
            ping->count = atoi(av[++i]);
            else
            {
                printf("ft_ping: invalid argument: '%s': must be a positive integer\n", av[i + 1]);
                exit(1);
            }
        }
        else if (strcmp(av[i], "-i") == 0 && i + 1 < ac)
        {
            if (is_valid_float(av[i + 1]))
                ping->interval = atof(av[++i]);
            else if (extract_non_numeric(av[i+1]) && strlen(av[i+1]) != strlen(extract_non_numeric(av[i+1])))
            {
                printf("ft_ping: option argument contains garbage: %s\nft_ping: this will become fatal error in the future\n", extract_non_numeric(av[i+1]));
                ping->interval = atof(av[++i]);
            }
            else
            {
                printf("ft_ping: option argument contains garbage:%s\nft_ping: this will become fatal error in the future.\n",av[i+1]);
                exit(1);
            }
        }
        else if (strcmp(av[i], "-n") == 0)
            ping->numeric = 1;
        else if (strcmp(av[i], "-s") == 0 && i + 1 < ac)
        {
            int size = atoi(av[++i]);
            if (is_positive_integer(av[i]) || (strcmp(av[i], "0") == 0))
                ping->packet_size = size;
            else if (size < MIN_PACKET_SIZE || size > MAX_PACKET_SIZE)
            {
                printf("ft_ping: invalid packet size '%d': must be between %d and %d bytes\n",
                                    size, MIN_PACKET_SIZE, MAX_PACKET_SIZE);
                                    exit(1);
            }
            else
            {
                printf("ft_ping: invalid argument: '%s': must be a valid integer\n", av[i]);
                exit(1);
                                
            }
        }
        else if (strcmp(av[i], "-q") == 0)
            ping->quiet = 1;
        else if (av[i][0] == '-' && check_flag(av[i]))
            ping->verbose++;
        else
        {
            if (check_ip(av[i]))
                ping->target[k++] = strdup(av[i]);
            else
            ping->hostname[t++] = strdup(av[i]);
        }
    }
    ping->target[k] = NULL;
}

int main(int ac, char **av)
{
    t_ping ping;
    ping.verbose = 0;
    ping.count = 0;
    ping.interval = 1;
    ping.numeric = 0;
    ping.packet_size = 56;
    ping.quiet = 0;
    ping.target = NULL;
   
    pars(ac, av, &ping);
    
    if (ping.hostname)
    {
        for (int i = 0; ping.hostname[i] != NULL; i++)
        extract_ip(&ping, i);
    }

    ft_ping(&ping);

    for (int j = 0; ping.hostname[j]; j++)
    free(ping.hostname[j]);
    for (int j = 0; ping.target[j]; j++)
    free(ping.target[j]);
    free(ping.hostname);
    free(ping.target);
    return 0;
}

