#include "../include/ft_nmap.h"
#include <netdb.h>

char *resolve_target(const char *input)
{
    if (!input)
        return NULL;

    struct in_addr addr;
    if (inet_pton(AF_INET, input, &addr) == 1)
        return strdup(input);

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(input, NULL, &hints, &res) != 0) 
    {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Error: cannot resolve '%s'", input);
        print_error(error_msg);
        return NULL;
    }

    char ip_str[INET_ADDRSTRLEN];
    struct sockaddr_in *sa = (struct sockaddr_in *)res->ai_addr;
    inet_ntop(AF_INET, &sa->sin_addr, ip_str, sizeof(ip_str));
    freeaddrinfo(res);
    return strdup(ip_str);
}

void load_targets_from_file(const char *path, Arguments *args)
{
    FILE *f = fopen(path, "r");
    if (!f) 
    {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Error: cannot open file '%s'", path);
        print_error(error_msg);
    }

    char line[1024];
    while (fgets(line, sizeof(line), f))
    {
        char *tok = strtok(line, " \t\r\n");
        while (tok)
        {
            if (args->target_count >= 1024) 
            {
                printf(" target limit (1024) \n");
                fclose(f);
                return;
            }
            args->targets[args->target_count++] = strdup(tok);
            tok = strtok(NULL, " \t\r\n");
        }
    }

    fclose(f);

    if (args->target_count == 0) 
    {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Error: no targets found in '%s'", path);
        print_error(error_msg);
    }
}

void parse_args(int argc, char **argv, Arguments *args)
{
    memset(args, 0, sizeof(*args));

    args->speedup    = 1;
    args->port_count = 0;
    for (int p = 1; p <= MAX_PORTS; p++)
        args->ports[args->port_count++] = p;

    args->scan_count = MAX_SCAN_TYPES;
    for (int s = 0; s < MAX_SCAN_TYPES; s++)
        args->scan_types[s] = (ScanType)s;

    char *raw_target  = NULL;
    int   target_flag = 0;   // 0=none, 1=positional, 2=--ip, 3=--hostname, 4=--file

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--ip") == 0)
        {
            if (++i >= argc)
                print_error("--ip needs a value"); 
            
            if (target_flag != 0) 
                print_error("Error: conflicting targets"); 

            struct in_addr addr;
            if (inet_pton(AF_INET, argv[i], &addr) != 1) 
            {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), 
                        "Error: '%s' is not a valid IPv4 address\n"
                        "       Use --hostname for domain names", argv[i]);
                print_error(error_msg);
            }
            raw_target  = argv[i];
            target_flag = 2;
        }
        else if (strcmp(argv[i], "--hostname") == 0)
        {
            if (++i >= argc)
                print_error("--hostname needs a value");   
            if (target_flag != 0) 
                print_error("Error: conflicting targets"); 
            raw_target  = argv[i];
            target_flag = 3;
        }
        else if (strcmp(argv[i], "--file") == 0)
        {
            if (++i >= argc)
                print_error("--file needs a value"); 
            
            if (target_flag != 0) 
                print_error("Error: conflicting targets"); 
            load_targets_from_file(argv[i], args);
            target_flag = 4;
        }
        else if (strcmp(argv[i], "--ports") == 0)
        {
            if (++i >= argc)
                print_error("--ports needs a value");
            args->port_count = 0;
            if (!is_valid_ports(argv[i], args->ports, &args->port_count)) 
            {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), "Invalid ports: '%s'", argv[i]);
                print_error(error_msg);
            }
        }
        else if (strcmp(argv[i], "--scan") == 0)
        {
            if (++i >= argc)
                print_error("--scan needs a value"); 
            
            args->scan_count = 0;
            char buf[128];
            strncpy(buf, argv[i], sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            char *tok = strtok(buf, ",");
            while (tok && args->scan_count < MAX_SCAN_TYPES) 
            {
                for (char *p = tok; *p; p++)
                    *p = (char)toupper((unsigned char)*p);
                if      (strcmp(tok, "SYN")  == 0) 
                    args->scan_types[args->scan_count++] = SCAN_SYN;
                else if (strcmp(tok, "NULL") == 0) 
                    args->scan_types[args->scan_count++] = SCAN_NULL;
                else if (strcmp(tok, "ACK")  == 0) 
                    args->scan_types[args->scan_count++] = SCAN_ACK;
                else if (strcmp(tok, "FIN")  == 0) 
                    args->scan_types[args->scan_count++] = SCAN_FIN;
                else if (strcmp(tok, "XMAS") == 0) 
                    args->scan_types[args->scan_count++] = SCAN_XMAS;
                else if (strcmp(tok, "UDP")  == 0) 
                    args->scan_types[args->scan_count++] = SCAN_UDP;
                else 
                { 
                    char error_msg[256];
                    snprintf(error_msg, sizeof(error_msg), "Unknown scan type: '%s'", tok);
                    print_error(error_msg); 
                }
                tok = strtok(NULL, ",");
            }
            if (args->scan_count == 0) 
                print_error("No valid scan types specified.");
        }
        else if (strcmp(argv[i], "--speedup") == 0)
        {
            if (++i >= argc) 
                print_error("--speedup needs a value"); 
            args->speedup = atoi(argv[i]);
            if (args->speedup < 1)
                args->speedup = 1;
            if (args->speedup > MAX_THREADS)
                args->speedup = MAX_THREADS;
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
            exit(0);
        else if (argv[i][0] != '-')
        {
            if (target_flag != 0)
                print_error("Error: conflicting targets"); 
            raw_target  = argv[i];
            target_flag = 1;
        }
        else 
        {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "Unknown option: '%s'", argv[i]);
            print_error(error_msg);
        }
    }

    if (target_flag != 4)
    {
        if (!raw_target) 
        {
            print_error("Error: no target specified\n"
                        "Usage: sudo ./ft_nmap [options] <ip|hostname>\n"
                        "       sudo ./ft_nmap --file <targets.txt> [options]");
        }
        args->ip       = resolve_target(raw_target);
        if (!args->ip) 
            print_error("Failed to resolve target");
        args->hostname = raw_target;

        args->targets[0]   = raw_target;
        args->target_count = 1;
    }
}