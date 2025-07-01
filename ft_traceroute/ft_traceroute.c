
#include "ft_traceroute.h"

uint16_t compute_checksum(void *b, int len)
{
    uint16_t *buf = b;
    uint32_t sum = 0;
    uint16_t result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(uint8_t *)buf;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

void print_hop(int ttl, struct sockaddr_in *addr, double rtts[3], int count, int resolve_dns)
{
    char host[NI_MAXHOST];
    const char *ip_str = inet_ntoa(addr->sin_addr);

    if (count == 0) {
        printf("%2d  * * *\n", ttl);
        return;
    }

    if (resolve_dns && getnameinfo((struct sockaddr *)addr, sizeof(*addr), host, sizeof(host), NULL, 0, NI_NAMEREQD) == 0) {
        printf("%2d  %s (%s)", ttl, host, ip_str);
    } else {
        printf("%2d  %s", ttl, ip_str);
    }

    for (int i = 0; i < count; i++) {
        printf("  %.3f ms", rtts[i]);
    }

    printf("\n");
}


int resolve_hostname(const char *hostname, struct sockaddr_in *addr)
{
    struct addrinfo hints, *res;
    ft_memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;

    if (getaddrinfo(hostname, NULL, &hints, &res) != 0)
    {
        perror("getaddrinfo");
        return -1;
    }

    ft_memcpy(addr, res->ai_addr, sizeof(struct sockaddr_in));
    freeaddrinfo(res);
    return 0;
}

int send_icmp_packet(int sockfd, struct sockaddr_in *dest, int ttl)
{
    char packet[sizeof(struct icmphdr) + 56];
    ft_memset(packet, 0, sizeof(packet));

    struct icmphdr *icmp_hdr = (struct icmphdr *)packet;
    icmp_hdr->type = ICMP_ECHO;
    icmp_hdr->code = 0;
    icmp_hdr->un.echo.id = getpid();
    icmp_hdr->un.echo.sequence = ttl;

    ft_memset(packet + sizeof(struct icmphdr), 'A', 56);

    icmp_hdr->checksum = compute_checksum((unsigned short *)packet, sizeof(packet));

    setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
    if (sendto(sockfd, packet, sizeof(packet), 0, (struct sockaddr *)dest, sizeof(*dest)) < 0)
    {
        perror("sendto");
        return -1;
    }
    return 0;
}
int receive_icmp_response(int sockfd, struct sockaddr_in *src, struct timeval *rtt, int *type, int *code, int expected_id, int expected_seq)
{
    char buffer[512];
    struct ip *ip_hdr = (struct ip *)buffer;
    struct icmphdr *icmp_hdr;
    socklen_t len = sizeof(*src);
    struct timeval start, end;

    gettimeofday(&start, NULL);
    
    int recv_len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)src, &len);
    if (recv_len < 0)
    {
        perror("recvfrom");
        return -1;
    }

    gettimeofday(&end, NULL);

    timersub(&end, &start, rtt);

    int ip_header_len = ip_hdr->ip_hl * 4;
    icmp_hdr = (struct icmphdr *)(buffer + ip_header_len);

    *type = icmp_hdr->type;
    *code = icmp_hdr->code;

   
    if (*type == ICMP_TIME_EXCEEDED)
    {
        struct ip *orig_ip = (struct ip *)(icmp_hdr + 1);
        int orig_ip_len = orig_ip->ip_hl * 4;
        struct icmphdr *orig_icmp = (struct icmphdr *)((char *)orig_ip + orig_ip_len);


        if (orig_icmp->un.echo.id != expected_id || orig_icmp->un.echo.sequence != expected_seq)
            return -1;  
    }
    else if (*type == ICMP_ECHOREPLY)
    {
        if (icmp_hdr->un.echo.id != expected_id || icmp_hdr->un.echo.sequence != expected_seq)
            return -1;
    }
    else
    {
        return -1;
    }

    return 0;
}

void ft_traceroute(t_traceroute *traceroute)
{
    char *target;
    struct sockaddr_in dest;
    int sockfd;

    if (traceroute->target)
        target = traceroute->target;

    if (resolve_hostname(target, &dest) < 0)
    {
        printf("Error resolving hostname\n");
        return ;
    }

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        perror("socket");
        return ;
    }

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    printf("Ft_Traceroute to %s (%s), %d hops max\n", target, inet_ntoa(dest.sin_addr), traceroute->max_ttl);

    for (int ttl = traceroute->first_ttl; ttl <= traceroute->max_ttl; ttl++)
    {
        double rtts[3] = {0};
        int responses = 0;
        struct sockaddr_in addr = {0};

        for (int probe = 0; probe < 3; probe++)
        {
            send_icmp_packet(sockfd, &dest, ttl);

            struct sockaddr_in resp_addr;
            struct timeval rtt;
            int type, code;

            int ret = receive_icmp_response(sockfd, &resp_addr, &rtt, &type, &code, getpid(), ttl);
            if (ret == 0)
            {
                addr = resp_addr;

                rtts[responses++] = (rtt.tv_sec * 1000.0) + (rtt.tv_usec / 1000.0);

                if (resp_addr.sin_addr.s_addr == dest.sin_addr.s_addr)
                {
                    print_hop(ttl, &addr, rtts, responses, traceroute->resolve_dns);
                    printf("Destination reached.\n");
                    close(sockfd);
                    return;
                }
            }
        }

        print_hop(ttl, &addr, rtts, responses, traceroute->resolve_dns);
    }

    close(sockfd);
}
