#include "../include/ft_nmap.h"


static const char *scanTypeStr[] = {"SYN", "NULL", "ACK", "FIN", "XMAS", "UDP"};


int check_type_Scan(char *scan)
{
    if (strcmp(scan, "SYN")  == 0) 
        return SCAN_SYN;
    if (strcmp(scan, "NULL") == 0) 
        return SCAN_NULL;
    if (strcmp(scan, "ACK")  == 0)
         return SCAN_ACK;
    if (strcmp(scan, "FIN")  == 0) 
        return SCAN_FIN;
    if (strcmp(scan, "XMAS") == 0) 
        return SCAN_XMAS;
    if (strcmp(scan, "UDP")  == 0) 
        return SCAN_UDP;

    fprintf(stderr, "Unknown scan type: '%s'\n", scan);
    exit(1);
}


static void print_help(const char *prog)
{
    printf(
        "\nUsage: sudo %s [OPTIONS] <target>\n"
        "\n"
        "ft_nmap — nmap-style TCP/UDP port scanner.\n"
        "\n"
        "Options:\n"
        "  --help, -h            Show this help and exit.\n"
        "  --ip <addr>           Target IPv4 address or hostname.\n"
        "--hostname <hostname>   Targe of hostname.\n"
        "--file <addr,hostname>  list of IPv4 addresses or hostnames.\n"
        "  --ports <spec>        Ports to scan (max MAX_PORTS).  Formats:\n"
        "                        22          single port\n"
        "                        22,80,443   comma list\n"
        "                        1-MAX_PORTS      range\n"
        "                        Default: 1-MAX_PORTS\n"
        "  --scan <types>        Comma-separated scan types:\n"
        "                        SYN  NULL  ACK  FIN  XMAS  UDP\n"
        "                        Default: all six types.\n"
        "  --speedup <n>         Parallel threads  (default: 1, max: 250).\n"
        "\n"
        "Examples:\n"
        "  sudo %s --ip 8.8.8.8\n"
        "  sudo %s --ip 10.0.0.1 --ports 22,80,443 --scan SYN,UDP\n"
        "  sudo %s --ip 192.168.1.1 --ports 1-MAX_PORTS --speedup 200\n"
        "\n"
        "Requires root (raw sockets).\n\n",
        prog, prog, prog, prog);
}


int main(int argc, char **argv)
{
    if (geteuid() != 0) {
        fprintf(stderr, "Error: ft_nmap requires root\n");
        return 1;
    }
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_help(argv[0]); return 0;
        }
    if (argc < 2) { print_help(argv[0]); return 1; }

    Arguments args;
    parse_args(argc, argv, &args);

    printf("\n=== Scan Configurations ===\n");
    printf("Targets              : %d host(s)\n", args.target_count);
    printf("Ports                : %d\n",          args.port_count);
    printf("Scans                : ");
    for (int i = 0; i < args.scan_count; i++)
        printf("%s ", scanTypeStr[args.scan_types[i]]);
    printf("\nThreads              : %d\n", args.speedup);
    printf("===========================\n");

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int t = 0; t < args.target_count; t++)
    {
        char *resolved = resolve_target(args.targets[t]);
        if (!resolved) {
            fprintf(stderr, "Skipping '%s': cannot resolve\n", args.targets[t]);
            continue;
        }

        printf("\nScanning %s (%s) ...\n", args.targets[t], resolved);

        perform_scan(resolved, args.targets[t],
                     args.ports, args.port_count,
                     args.scan_types, args.scan_count,
                     args.speedup);

        free(resolved);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (double)(end.tv_sec  - start.tv_sec) +
                     (double)(end.tv_nsec - start.tv_nsec) / 1e9;
    if (elapsed < 60.0)
        printf("\nTotal scan time: %.2f secs\n", elapsed);
    else
        printf("\nTotal scan time: %dm %.2fs\n",
               (int)(elapsed/60), elapsed - (int)(elapsed/60)*60.0);


    for (int t = 0; t < args.target_count; t++)
        if (args.targets[t] != args.targets[0] || args.target_count > 1)
            free(args.targets[t]);

    return 0;
}