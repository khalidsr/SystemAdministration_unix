#ifndef FT_NMAP_H
#define FT_NMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netdb.h>


#define MAX_THREADS     250
#define MAX_PORTS       1024
#define MAX_SCAN_TYPES  6


#define RESULT_LEN      32      // "SYN(Open|Filtered)"            
#define RECV_BUF        65535


typedef enum {
    SCAN_SYN,
    SCAN_NULL,
    SCAN_ACK,
    SCAN_FIN,
    SCAN_XMAS,
    SCAN_UDP,
} ScanType;

typedef struct {
    int  port;
    char results[MAX_SCAN_TYPES][RESULT_LEN]; // "SYN(Open)" .          
} MultiScanResult;


struct pseudo_hdr {
    uint32_t src, dst;
    uint8_t  zero, proto;
    uint16_t tcp_len;
};

typedef struct {
    struct sockaddr_in  target;
    int                 port;
    int                 port_index;   // unique index
    ScanType            scan_type;
    int                 scan_index;   // which column in results[]
    MultiScanResult    *result;
    pthread_mutex_t    *mutex;
} ThreadArgs;

typedef struct {
    char        *ip;             
    char        *hostname;        
    char        *targets[1024];   
    int          target_count;     
    int          ports[MAX_PORTS];
    int          port_count;
    ScanType     scan_types[MAX_SCAN_TYPES];
    int          scan_count;
    int          speedup;
} Arguments;


void perform_scan(char *ip, char *hostname, int *ports, int port_count, ScanType *scan_types, int scan_count, int speedup);
char *derive_conclusion(MultiScanResult *r, ScanType *scan_types, int scan_count);

void  parse_args(int argc, char **argv, Arguments *args);
char *resolve_target(const char *input);
void load_targets_from_file(const char *path, Arguments *args);
void print_error(char *str);

void  ft_free(char **ptr);
char *ft_strdup(const char *s1);
char *ft_substr(const char *s, unsigned int start, size_t len);
char **ft_split(char const *s, char c);
bool  is_valid_ip(const char *ip);
bool  is_valid_ports(const char *ports, int *port_list, int *port_count);


int   check_type_Scan(char *scan);

unsigned short checksum(void *b, int len);

const char *syn_connect_probe(struct sockaddr_in *target, int port);

#endif