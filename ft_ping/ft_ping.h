#ifndef FT_PING_H
# define FT_PING_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <sys/time.h>
#include <signal.h>
#include <math.h>
#define MIN_PACKET_SIZE 0
#define MAX_PACKET_SIZE 65507



typedef struct s_ping {
char **target;
char **hostname;
int count;
double interval;
int numeric;
int packet_size;
int quiet;
int verbose;
} t_ping;

typedef struct s_packets {
    int packets_sent;
    int packets_received;
    double min_rtt;
    double max_rtt;
    double total_rtt;
    double avg_rtt;
    int packet_loss;
    double rtt_times[1000];

} t_packets;

typedef struct s_global {
    t_packets packets;
    struct timeval start_time;
    struct timeval end_time;
    t_ping ping;
    char *target_name;
} t_global;

struct icmp_hdr
{
    uint8_t type;       
    uint8_t code;      
    uint16_t checksum; 
    uint16_t id;        
    uint16_t sequence;
};

char	**ft_split(char const *s, char c);
char	*ft_strdup(const char *s1);
char	*ft_substr(const char *s, unsigned int start, size_t len);
int	ft_len(char const *s, char c);
void ft_free(char **ptr);
int ft_ping(t_ping *ping);
uint16_t calculate_checksum(void *b, int len);
void construct_icmp_packet(char *packet, int sequence, int payload_size);

void send_icmp_packet(const char *hostname, int verbose, const char *ip_address, t_ping *ping, int flag);
int receive_icmp_reply(int sockfd, int sequence, int *ttl);

void handle_sigint(int sig);

void print_help();
void extract_ip(t_ping *ping, int index);
#endif

