#ifndef FT_TRACEROUTE_H
# define FT_TRACEROUTE_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netinet/ip.h>   
#include <netinet/ip_icmp.h>  
#include <errno.h>
typedef struct s_traceroute
{
char *target;
int     help;
int resolve_dns;    
int first_ttl; 
int max_ttl;         
} t_traceroute;

typedef struct s_icmp
{
    uint8_t type;       
    uint8_t code;      
    uint16_t checksum; 
    uint16_t id;
    uint16_t sequence;

}t_icmp;

int ft_atoi(const char *str);
char	**ft_split(char const *s, char c);

char	*ft_strdup(const char *s1);
char	*ft_substr(const char *s, unsigned int start, size_t len);
int	    ft_len(char const *s, char c);
void    ft_free(char **ptr);
size_t ft_strlen(const char *str);
void print_help();
int	ft_strcmp(const char *s1, const char *s2);
int	ft_isdigit(int c);

int receive_icmp_response(int sockfd, struct sockaddr_in *src, struct timeval *rtt, int *type, int *code, int expected_id, int expected_seq);
int send_icmp_packet(int sockfd, struct sockaddr_in *dest, int ttl);
int resolve_hostname(const char *hostname, struct sockaddr_in *addr);

void print_hop(int ttl, struct sockaddr_in *addr, double rtts[3], int count, int resolve_dns);
void ft_traceroute(t_traceroute *traceroute);
void	*ft_memset(void *b, int c, size_t len);
void	*ft_memcpy(void *dest, const void *src, size_t n);
uint16_t compute_checksum(void *b, int len);
#endif

