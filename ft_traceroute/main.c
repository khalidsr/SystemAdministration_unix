#include "ft_traceroute.h"



int parse_args(int argc, char **argv, t_traceroute *tr)
{
    tr->resolve_dns = 1;
    tr->first_ttl = 1;
    tr->max_ttl = 30;

    for (int i = 1; i < argc; i++) 
    {
        if (ft_strcmp(argv[i], "--help") == 0)
        {
            print_help();
            exit(0);
        } 
        else if (ft_strcmp(argv[i], "-n") == 0) 
            tr->resolve_dns = 0;
        else if (ft_strcmp(argv[i], "-f") == 0) 
        {
            if (i + 1 >= argc) 
                return -1;
            tr->first_ttl = atoi(argv[++i]);
        } 
        else if (ft_strcmp(argv[i], "-m") == 0) 
        {
            if (i + 1 >= argc) 
                return -1;
            tr->max_ttl = atoi(argv[++i]);
        } 
        else if (!tr->target) 
            tr->target = argv[i];
        else 
            return -1;
    }

    if (!tr->target)
        return -1;

    return 0;
}

int main(int ac, char **av)
{
    t_traceroute tr;
    ft_memset(&tr, 0, sizeof(t_traceroute));

    if (parse_args(ac, av, &tr) < 0)
    {
        print_help();
        return 1;
    }
    ft_traceroute(&tr);
    return 0;
}
