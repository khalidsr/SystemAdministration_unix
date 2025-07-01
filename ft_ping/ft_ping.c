#include "ft_ping.h"

t_global g;

void handle_sigint(int sig)
{
    (void) sig;
    gettimeofday(&g.end_time, NULL);
    double total_time = (g.end_time.tv_sec - g.start_time.tv_sec) * 1000.0;
    total_time += (g.end_time.tv_usec - g.start_time.tv_usec) / 1000.0;
    
    printf("\n--- %s ping statistics ---\n", g.target_name);
    printf("%d packets transmitted, %d received, %d%% packet loss, time %.0fms\n", 
           g.packets.packets_sent, g.packets.packets_received,
           ((g.packets.packets_sent - g.packets.packets_received) / g.packets.packets_sent) * 100,
           total_time);

    if (g.packets.packets_received > 0)
    {
        double avg_rtt = g.packets.total_rtt / g.packets.packets_received;

        double mdev = 0.0;
        for (int i = 0; i < g.packets.packets_received; i++)
        {
            double diff = g.packets.rtt_times[i] - avg_rtt;
            mdev += diff * diff;
        }
        mdev = sqrt(mdev / g.packets.packets_received);

        printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n", 
            g.packets.min_rtt, avg_rtt, g.packets.max_rtt, mdev);
    }
    else
    {
        printf("rtt min/avg/max/mdev = 0.0/0.0/0.0/0.0 ms\n");
    }
    free(g.target_name);

    exit(0);
}


uint16_t calculate_checksum(void *b, int len)
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
void construct_icmp_packet(char *packet, int sequence, int payload_size)
{
    struct icmp_hdr *icmp = (struct icmp_hdr *) packet;

    icmp->type = 8;    
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->id = getpid() & 0xFFFF;
    icmp->sequence = sequence;

    char *payload = packet + sizeof(struct icmp_hdr);
    memset(payload, 65, payload_size); 
    int packet_size = sizeof(struct icmp_hdr) + payload_size;
    icmp->checksum = calculate_checksum(packet, packet_size);
}

int resolve_hostname_to_ip(const char *hostname, char *ip_address, size_t ip_len)
{
    struct addrinfo hints, *res, *p;
    int status;
    void *addr;
    char ipstr[INET_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;

    status = getaddrinfo(hostname, NULL, &hints, &res);
    if (status != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 0;
    }

    for (p = res; p != NULL; p = p->ai_next)
    {
        struct sockaddr_in *ipv4;
        ipv4 = (struct sockaddr_in *)p->ai_addr;
        addr = &(ipv4->sin_addr);
        
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);

        strncpy(ip_address, ipstr, ip_len - 1);
        ip_address[ip_len - 1] = '\0';

        break;
    }

    freeaddrinfo(res); 

    return 1;
}
void initial_packet(t_packets *packets)
{
    packets->packets_sent = 0;
    packets->packets_received = 0;
    packets->min_rtt = 1000000.0;
    packets->max_rtt = 0.0;
    packets->total_rtt = 0.0;
    packets->avg_rtt =0;
    packets->packet_loss = 0; 
}

void send_icmp_packet(const char *hostname, int verbose, const char *ip_address, t_ping *ping, int flag)
{
    int sockfd;
    char packet[1500]; 
    struct sockaddr_in dest;
    struct timeval start, end;
    double rtt;
    int ttl;
    
    int payload_size = ping->packet_size;
    
    initial_packet(&g.packets);
    signal(SIGINT, handle_sigint);

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        perror("socket");
        exit(1);
    }

    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    inet_pton(AF_INET, ip_address, &dest.sin_addr);

    int icmp_header_size = sizeof(struct icmp_hdr);
    int ip_header_size = 20;
    int icmp_packet_size = icmp_header_size + payload_size;
    int total_size = ip_header_size + icmp_packet_size;
    if (verbose)
    {
        printf("ping: sock4.fd: %d (socktype: SOCK_RAW), hints.ai_family: AF_UNSPEC\n\n", sockfd);
        if (hostname != NULL)
            printf("ai->ai_family: AF_INET, ai->ai_canonname: '%s'\n", hostname);
        else
            printf("ai->ai_family: AF_INET, ai->ai_canonname: '%s'\n", ip_address);
    }

    if (hostname != NULL)
    {
        g.target_name= strdup(hostname);
        printf("FT_PING %s (%s) %d(%d) bytes of data.\n", hostname, ip_address, payload_size, total_size);
    }
    else
    {
        g.target_name= strdup(ip_address);
        printf("FT_PING %s %d(%d) bytes of data.\n", ip_address, payload_size, total_size);
    }
        
    gettimeofday(&g.start_time, NULL);

    int count = 0;
    for (int seq = 1; (ping->count == 0 || count < ping->count); seq++)
    {
        construct_icmp_packet(packet, seq, payload_size);
        gettimeofday(&start, NULL);
        g.packets.packets_sent++;

        int packet_size = sizeof(struct icmp_hdr) + payload_size;
        if (sendto(sockfd, packet, packet_size, 0, (struct sockaddr *)&dest, sizeof(dest)) <= 0)
        {
            perror("sendto");
            continue;
        }
        if (!flag)
        {
            if (receive_icmp_reply(sockfd, seq, &ttl))
            {
                gettimeofday(&end, NULL);
                rtt = (end.tv_sec - start.tv_sec) * 1000.0;
                rtt += (end.tv_usec - start.tv_usec) / 1000.0;
                g.packets.packets_received++;
                g.packets.total_rtt += rtt;
                g.packets.rtt_times[g.packets.packets_received - 1] = rtt;

                if (rtt < g.packets.min_rtt)
                g.packets.min_rtt = rtt;
                if (rtt > g.packets.max_rtt)
                g.packets.max_rtt = rtt;

                if (!ping->quiet)
                {
                    printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.2f ms\n",
                        packet_size, ip_address, seq, ttl, rtt);
                }
            }
            else
            {
                if (!ping->quiet)
                    printf("Request timeout for icmp_seq %d\n", seq);
            }

            count++;
            if (ping->count > 0 && count >= ping->count)
                break;
        }
        usleep(ping->interval * 1000000);
    }

    close(sockfd);
    handle_sigint(0);
    
}


int receive_icmp_reply(int sockfd, int sequence, int *ttl)
{
    char buffer[1024];
    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);
    ssize_t bytes_received;

    bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&sender, &sender_len);
    if (bytes_received < 0)
    {
        perror("recvfrom");
        return 0;
    }

    struct iphdr *ip = (struct iphdr *)buffer;
    struct icmp_hdr *icmp = (struct icmp_hdr *)(buffer + (ip->ihl * 4));

    *ttl = ip->ttl;

    if (icmp->type == 0 && icmp->code == 0)
    {
        if (icmp->id == (getpid() & 0xFFFF) && icmp->sequence == sequence)
        {
            return 1;
        }
    }
    return 0;
}


void extract_ip(t_ping *ping, int index)
{
    int i = 0;
    while (ping->target[i])
        i++;
 
    char ip_address[INET_ADDRSTRLEN];
    if (resolve_hostname_to_ip(ping->hostname[index], ip_address, sizeof(ip_address)))
    {
        ping->target[i] = ft_strdup(ip_address);
        ping->target[++i] = NULL;
    }
    else
    {
        printf("ft_ping: %s: Name or service not known:\n", ping->hostname[index]);
        exit(1);
    }
}

int ft_ping(t_ping *ping)
{
    int len_target = 0;

    while (ping->target[len_target])
        len_target++;
    int len_host = len_target;

    if (len_host > 1)
        len_host -=1;
    if (len_target == 1)
        send_icmp_packet(ping->hostname[0], ping->verbose, ping->target[0], ping,0);
    else
        send_icmp_packet(ping->hostname[len_host], ping->verbose, ping->target[len_target-1], ping, 1);
    return 0;

}

