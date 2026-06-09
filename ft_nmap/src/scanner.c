#include "../include/ft_nmap.h"


char *scanTypeStr[] = {"SYN", "NULL", "ACK", "FIN", "XMAS", "UDP"};


const char *g_ip = NULL;


// struct pseudo_hdr {
//     uint32_t src, dst;
//     uint8_t  zero, proto;
//     uint16_t tcp_len;
// };


unsigned short checksum(void *b, int len)
{
    unsigned short *buf = b;
    unsigned int sum = 0;
    for (; len > 1; len -= 2) sum += *buf++;
    if (len == 1)              sum += *(unsigned char *)buf;
    sum  = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

const char *resolve_service(int port)
{
    switch (port) 
    {
        case 21:  return "ftp";
        case 22:  return "ssh";
        case 23:  return "telnet";
        case 25:  return "smtp";
        case 53:  return "domain";
        case 67:  return "dhcps";
        case 69:  return "tftp";
        case 79:  return "finger";
        case 80:  return "http";
        case 110: return "pop3";
        case 111: return "rpcbind";
        case 123: return "ntp";
        case 137: return "netbios-ns";
        case 138: return "netbios-dgm";
        case 139: return "netbios-ssn";
        case 143: return "imap";
        case 161: return "snmp";
        case 162: return "snmptrap";
        case 389: return "ldap";
        case 443: return "https";
        case 445: return "ms-ds";
        case 500: return "isakmp";
        case 514: return "syslog";
        case 993: return "imaps";
        case 995: return "pop3s";
        case 2049: return "nfs";
        case 5900: return "vnc";
        case 9100: return "jetdirect";
        default:  return "unassigned";
    }
}


static long long get_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

//    UDP probe payloads

static int build_udp_probe(int port, char *buf, int buflen)
{
    memset(buf, 0, buflen);
    switch (port) {
        case 53: {
            unsigned char dns[] = 
            {
                0x00,0x1e,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
                0x07,'v','e','r','s','i','o','n',
                0x04,'b','i','n','d',0x00,
                0x00,0x10,0x00,0x03
            };
            int l = (int)sizeof(dns); if (l > buflen) l = buflen;
            memcpy(buf, dns, l); return l;
        }
        case 123: {
            unsigned char ntp[48] = {0}; ntp[0] = 0x1b;
            memcpy(buf, ntp, 48); return 48;
        }
        case 161: {
            unsigned char s[] = {
                0x30,0x26,0x02,0x01,0x00,0x04,0x06,'p','u','b','l','i','c',
                0xa0,0x19,0x02,0x01,0x01,0x02,0x01,0x00,0x02,0x01,0x00,
                0x30,0x0e,0x30,0x0c,0x06,0x08,
                0x2b,0x06,0x01,0x02,0x01,0x01,0x01,0x00,0x05,0x00
            };
            int l = (int)sizeof(s); if (l > buflen) l = buflen;
            memcpy(buf, s, l); return l;
        }
        default: return 0;
    }
}


//  derive_conclusion
char *derive_conclusion(MultiScanResult *r, ScanType *scan_types, int scan_count)
{
    int has_open=0, has_openf=0, has_unf=0, has_closed=0, has_filt=0;
    int syn_open=0, syn_closed=0;

    for (int s = 0; s < scan_count; s++) 
    {
        const char *res = r->results[s];
        if (strstr(res, "(Open)"))      
            has_open   = 1;
        else if (strstr(res, "Open|Filt"))
            has_openf  = 1;
        else if (strstr(res, "Unfiltered"))
            has_unf    = 1;
        else if (strstr(res, "(Closed)"))
            has_closed = 1;
        else if (strstr(res, "(Filtered)"))
            has_filt   = 1;

        // SYN is the most reliable TCP scan — track its result separately 
        if (scan_types[s] == SCAN_SYN) 
        {
            if (strstr(res, "(Open)"))
                syn_open   = 1;
            if (strstr(res, "(Closed)"))
                syn_closed = 1;
        }
    }
    (void)has_filt;

    if (syn_open)               
        return "Open";
    if (syn_closed)
        return "Closed";
    if (has_open)               
        return "Open";
    if (has_unf && !has_closed)
        return "Unfiltered";
    if (has_openf)
        return "Open|Filtered";
    if (has_closed)
        return "Closed";
    return "Filtered";
}


void send_packet(int sock, struct sockaddr_in *target, int port, ScanType scan_type, uint16_t src_port)
{
    if (scan_type == SCAN_UDP) 
    {
        int udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (udp_sock < 0) 
        { 
            perror("UDP socket"); 
            return; 
        }
        struct sockaddr_in t = *target;
        t.sin_port = htons((uint16_t)port);
        char probe[512];
        int plen = build_udp_probe(port, probe, sizeof(probe));
        sendto(udp_sock, probe, (size_t)plen, 0,
               (struct sockaddr *)&t, sizeof(t));
        close(udp_sock);
        return;
    }

    char packet[sizeof(struct iphdr) + sizeof(struct tcphdr)];
    memset(packet, 0, sizeof(packet));
    struct iphdr  *iph  = (struct iphdr *)packet;
    struct tcphdr *tcph = (struct tcphdr *)(packet + sizeof(struct iphdr));

    iph->version  = 4;  iph->ihl = 5;  iph->tos = 0;
    iph->tot_len  = htons(sizeof(packet));
    iph->id       = htons((uint16_t)(rand() & 0xFFFF));
    iph->frag_off = 0;  iph->ttl = 64;
    iph->protocol = IPPROTO_TCP;  iph->check = 0;
    iph->saddr    = inet_addr(g_ip);
    iph->daddr    = target->sin_addr.s_addr;

    tcph->source  = htons(src_port);
    tcph->dest    = htons((uint16_t)port);
    tcph->seq     = htonl((uint32_t)rand());
    tcph->ack_seq = 0;  tcph->doff = 5;
    tcph->window  = htons(5840);  tcph->check = 0;

    switch (scan_type) 
    {
        case SCAN_SYN:  tcph->syn = 1; break;
        case SCAN_NULL: break;
        case SCAN_ACK:  tcph->ack = 1; tcph->ack_seq = htonl(1); break;
        case SCAN_FIN:  tcph->fin = 1; break;
        case SCAN_XMAS: tcph->fin = 1; tcph->psh = 1; tcph->urg = 1; break;
        default: break;
    }

    // TCP checksum over pseudo-header + TCP header 
    struct { struct pseudo_hdr ph; struct tcphdr th; } cs;
    memset(&cs, 0, sizeof(cs));
    cs.ph.src = iph->saddr;  cs.ph.dst = iph->daddr;
    cs.ph.proto = IPPROTO_TCP;
    cs.ph.tcp_len = htons(sizeof(struct tcphdr));
    memcpy(&cs.th, tcph, sizeof(struct tcphdr));
    tcph->check = checksum(&cs, sizeof(cs));
    iph->check  = checksum(iph, sizeof(struct iphdr));

    if (sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)target, sizeof(*target)) < 0)
        perror("sendto");
}


//    syn_connect_probe
 
//    Fallback for SYN scan when raw socket gets no reply:
//    try a real connect() with a very short timeout.
//    connect() succeeds  → Open
//    ECONNREFUSED        → Closed
//    ETIMEDOUT/other     → Filtered
 
//    This is needed because:
//      - Some firewalls (cloud VMs, GCP/AWS) strip raw packets
//     - The kernel may send RST to SYN-ACK before our raw socket sees it
//      - Remote firewalls may only respond to "real" TCP handshakes

const char *syn_connect_probe(struct sockaddr_in *target, int port)
{
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) 
        return NULL; 

    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in dst = *target;
    dst.sin_port = htons((uint16_t)port);

    connect(s, (struct sockaddr *)&dst, sizeof(dst));

    // waiting to 2 seconds with select
    fd_set wfds, efds;
    FD_ZERO(&wfds); FD_SET(s, &wfds);
    FD_ZERO(&efds); FD_SET(s, &efds);
    struct timeval tv = {2, 0};
    int rc = select(s + 1, NULL, &wfds, &efds, &tv);

    const char *verdict = NULL;
    if (rc > 0) 
    {
        int err = 0;
        socklen_t len = sizeof(err);
        getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len);
        if (err == 0)              verdict = "Open";
        else if (err == ECONNREFUSED) verdict = "Closed";
        else                       verdict = "Filtered";
    } 
    else 
    {
        verdict = "Filtered"; /* timeout */
    }

    close(s);
    return verdict;
}



void *scan_port_thread(void *arg)
{
    ThreadArgs *targs = (ThreadArgs *)arg;


    uint16_t src_port = (uint16_t)(10000 + (targs->port_index % 50000));

    char result_str[RESULT_LEN];
    memset(result_str, 0, sizeof(result_str));

    // UDP path
 
    if (targs->scan_type == SCAN_UDP)
    {
        int udp_raw   = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
        int icmp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

        if (udp_raw < 0 || icmp_sock < 0) {
            perror("socket(UDP raw)");
            if (udp_raw  >= 0) close(udp_raw);
            if (icmp_sock >= 0) close(icmp_sock);
            pthread_exit(NULL);
        }

        // send probe
        send_packet(-1, &targs->target, targs->port, SCAN_UDP, 0);

        long long deadline = get_ms() + 3000;
        int got = 0;

        while (!got) 
        {
            long long now    = get_ms();
            int       remain = (int)(deadline - now);
            if (remain <= 0) 
                break;

            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(udp_raw, &rfds);
            FD_SET(icmp_sock, &rfds);
            int maxfd = (udp_raw > icmp_sock ? udp_raw : icmp_sock) + 1;
            struct timeval tv = { remain / 1000, (remain % 1000) * 1000 };
            if (select(maxfd, &rfds, NULL, NULL, &tv) <= 0) break;

            char buf[RECV_BUF];
            struct sockaddr_in sender;
            socklen_t slen = sizeof(sender);

            //  UDP reply -> Open
            if (FD_ISSET(udp_raw, &rfds)) 
            {
                ssize_t n = recvfrom(udp_raw, buf, sizeof(buf), 0, (struct sockaddr *)&sender, &slen);
                if (n >= (int)sizeof(struct iphdr)) 
                {
                    struct iphdr *iph = (struct iphdr *)buf;
                    if (iph->saddr == targs->target.sin_addr.s_addr && iph->protocol == IPPROTO_UDP) 
                    {
                        int ihl = iph->ihl * 4;
                        if (n >= ihl + (int)sizeof(struct udphdr)) 
                        {
                            struct udphdr *uh = (struct udphdr *)(buf + ihl);
                            if (ntohs(uh->source) == (uint16_t)targs->port) 
                            {
                                snprintf(result_str, sizeof(result_str),
                                         "%s(Open)", scanTypeStr[SCAN_UDP]);
                                got = 1;
                            }
                        }
                    }
                }
            }

    
            if (!got && FD_ISSET(icmp_sock, &rfds)) 
            {
                ssize_t n = recvfrom(icmp_sock, buf, sizeof(buf), 0, (struct sockaddr *)&sender, &slen);
                if (n >= (int)sizeof(struct iphdr)) 
                {
                    struct iphdr *iph = (struct iphdr *)buf;
                    if (iph->protocol == IPPROTO_ICMP) 
                    {
                        int ihl = iph->ihl * 4;
                        if (n >= ihl + (int)sizeof(struct icmphdr)) 
                        {
                            struct icmphdr *ic = (struct icmphdr *)(buf + ihl);
                            if (ic->type == ICMP_DEST_UNREACH) 
                            {
                                int off = ihl + (int)sizeof(struct icmphdr);
                                if (n >= off + (int)sizeof(struct iphdr) + 4) 
                                {
                                    struct iphdr *inner = (struct iphdr *)(buf + off);
                                    uint16_t *tp = (uint16_t *)(buf + off + inner->ihl * 4);
                                    if (ntohs(tp[1]) == (uint16_t)targs->port) 
                                    {
                                        if (ic->code == ICMP_PORT_UNREACH)
                                            snprintf(result_str, sizeof(result_str), "%s(Closed)", scanTypeStr[SCAN_UDP]);
                                        else
                                            snprintf(result_str, sizeof(result_str), "%s(Filtered)", scanTypeStr[SCAN_UDP]);
                                        got = 1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        if (!got)
            snprintf(result_str, sizeof(result_str), "%s(Open|Filtered)", scanTypeStr[SCAN_UDP]);

        close(udp_raw);
        close(icmp_sock);
    }

 
    //    TCP path  (SYN / NULL / ACK / FIN / XMAS)

    else
    {
        int tcp_sock  = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
        int icmp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

        if (tcp_sock < 0) 
        {
            perror("socket(TCP raw)");
            if (icmp_sock >= 0) close(icmp_sock);
            pthread_exit(NULL);
        }
        int one = 1;
        setsockopt(tcp_sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one));

        // send probe
        send_packet(tcp_sock, &targs->target, targs->port, targs->scan_type, src_port);

        long long deadline = get_ms() + 3000;
        int got = 0;

        while (!got) 
        {
            long long now    = get_ms();
            int       remain = (int)(deadline - now);
            if (remain <= 0) 
                break;

            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(tcp_sock, &rfds);
            if (icmp_sock >= 0) 
                FD_SET(icmp_sock, &rfds);
            int maxfd = tcp_sock;
            if (icmp_sock > maxfd) 
                maxfd = icmp_sock;
            maxfd++;

            struct timeval tv = { remain / 1000, (remain % 1000) * 1000 };
            if (select(maxfd, &rfds, NULL, NULL, &tv) <= 0) 
                break;

            char buf[RECV_BUF];
            struct sockaddr_in sender;
            socklen_t slen = sizeof(sender);

            // TCP response 
            if (FD_ISSET(tcp_sock, &rfds)) 
            {
                ssize_t n = recvfrom(tcp_sock, buf, sizeof(buf), 0, (struct sockaddr *)&sender, &slen);
                if (n >= (int)sizeof(struct iphdr)) 
                {
                    struct iphdr *iph = (struct iphdr *)buf;
                    // filter: must be from target, must be TCP 
                    if (iph->saddr == targs->target.sin_addr.s_addr && iph->protocol == IPPROTO_TCP)
                    {
                        int ihl = iph->ihl * 4;
                        if (n >= ihl + (int)sizeof(struct tcphdr))
                        {
                            struct tcphdr *tcph = (struct tcphdr *)(buf + ihl);
                            // filter: must be response to our port 
                            if (ntohs(tcph->source) == (uint16_t)targs->port) 
                            {
                                got = 1;
                                if (targs->scan_type == SCAN_SYN) 
                                {
                                    if (tcph->syn && tcph->ack)
                                        snprintf(result_str, sizeof(result_str), "%s(Open)", scanTypeStr[SCAN_SYN]);
                                    else if (tcph->rst)
                                        snprintf(result_str, sizeof(result_str), "%s(Closed)", scanTypeStr[SCAN_SYN]);
                                    else
                                        snprintf(result_str, sizeof(result_str), "%s(Filtered)", scanTypeStr[SCAN_SYN]);

                                } 
                                else if (targs->scan_type == SCAN_ACK) 
                                {
                                    if (tcph->rst)
                                        snprintf(result_str, sizeof(result_str), "%s(Unfiltered)", scanTypeStr[SCAN_ACK]);
                                    else
                                        snprintf(result_str, sizeof(result_str), "%s(Filtered)", scanTypeStr[SCAN_ACK]);

                                } 
                                else 
                                {
                                    // NULL / FIN / XMAS
                                    if (tcph->rst)
                                        snprintf(result_str, sizeof(result_str), "%s(Closed)", scanTypeStr[targs->scan_type]);
                                    else
                                        snprintf(result_str, sizeof(result_str), "%s(Open)", scanTypeStr[targs->scan_type]);
                                }
                            }
                        }
                    }
                }
            }

            // ICMP response → Filtered
            // Same as UDP: don't filter by saddr — ICMP unreachable may
            // come from an intermediate router.
            //  Verify inner TCP dest port == our port instead.           
            if (!got && icmp_sock >= 0 && FD_ISSET(icmp_sock, &rfds)) 
            {
                ssize_t n = recvfrom(icmp_sock, buf, sizeof(buf), 0, (struct sockaddr *)&sender, &slen);
                if (n >= (int)sizeof(struct iphdr)) 
                {
                    struct iphdr *iph = (struct iphdr *)buf;
                    if (iph->protocol == IPPROTO_ICMP) 
                    {
                        int ihl = iph->ihl * 4;
                        if (n >= ihl + (int)sizeof(struct icmphdr))
                        {
                            struct icmphdr *ic = (struct icmphdr *)(buf + ihl);
                            if (ic->type == ICMP_DEST_UNREACH || ic->type == ICMP_TIME_EXCEEDED) 
                            {
                                int off = ihl + (int)sizeof(struct icmphdr);
                                if (n >= off + (int)sizeof(struct iphdr) + 4) 
                                {
                                    struct iphdr *inner = (struct iphdr *)(buf + off);
                                    if (inner->protocol == IPPROTO_TCP) 
                                    {
                                        uint16_t *tp = (uint16_t *)(buf + off + inner->ihl * 4);
                                        // tp[1] = dest port of original TCP packet 
                                        if (ntohs(tp[1]) == (uint16_t)targs->port) 
                                        {
                                            snprintf(result_str, sizeof(result_str), "%s(Filtered)", scanTypeStr[targs->scan_type]);
                                            got = 1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } 

        
        // SYN scan: always confirm with connect().
         
        //  Why we can't trust raw RST alone on LAN:
        //  - Target sends SYN-ACK → kernel sees it first, sends RST
        //  - Our raw socket reads that RST and thinks "Closed"
        //  - But the port IS open — the kernel's RST was the handshake cleanup
         
        //  connect() is the ground truth:
        //  success      → Open
        //  ECONNREFUSED → Closed  (real RST with no prior SYN-ACK)
        //  timeout      → Filtered
       
        //  For NULL/FIN/XMAS/ACK: no-response states are correct per nmap spec.
        
        if (targs->scan_type == SCAN_SYN) 
        {
            const char *v = syn_connect_probe(&targs->target, targs->port);
            if (v)
                snprintf(result_str, sizeof(result_str), "%s(%s)", scanTypeStr[SCAN_SYN], v);
            else
                snprintf(result_str, sizeof(result_str), "%s(Filtered)", scanTypeStr[SCAN_SYN]);
        } 
        else if (!got) 
        {
            switch (targs->scan_type) 
            {
                case SCAN_NULL:
                case SCAN_FIN:
                case SCAN_XMAS:
                    snprintf(result_str, sizeof(result_str), "%s(Open|Filtered)", scanTypeStr[targs->scan_type]);
                    break;
                default: // ACK
                    snprintf(result_str, sizeof(result_str), "%s(Filtered)", scanTypeStr[targs->scan_type]);
                    break;
            }
        }

        close(tcp_sock);
        if (icmp_sock >= 0) close(icmp_sock);
    }

    //  write result under lock 
    pthread_mutex_lock(targs->mutex);
    snprintf(targs->result->results[targs->scan_index],
             RESULT_LEN, "%s", result_str);
    pthread_mutex_unlock(targs->mutex);

    pthread_exit(NULL);
}


void perform_scan(char *ip, char *hostname, int *ports, int port_count, ScanType *scan_types, int scan_count, int speedup)
{
    if (speedup < 1)
        speedup = 1;
    if (speedup > MAX_THREADS)
        speedup = MAX_THREADS;
    g_ip = ip;
    (void) hostname;
    int total = port_count * scan_count;

    pthread_t       *threads = calloc((size_t)speedup, sizeof(pthread_t));
    int             *alive   = calloc((size_t)speedup, sizeof(int));
    ThreadArgs      *targs   = calloc((size_t)total,   sizeof(ThreadArgs));
    MultiScanResult *results = calloc((size_t)port_count, sizeof(MultiScanResult));
    pthread_mutex_t  mutex;

    if (!threads || !alive || !targs || !results) 
    {
        perror("calloc"); 
        return;
    }
    pthread_mutex_init(&mutex, NULL);

    struct sockaddr_in target;
    memset(&target, 0, sizeof(target));
    target.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &target.sin_addr);

    // init results to "N/A"
    for (int i = 0; i < port_count; i++) 
    {
        results[i].port = ports[i];
        for (int s = 0; s < scan_count; s++)
            strncpy(results[i].results[s], "N/A", RESULT_LEN - 1);
    }

    int launched = 0, joined = 0;

    for (int i = 0; i < port_count; i++) 
    {
        for (int s = 0; s < scan_count; s++) 
        {
            int idx  = i * scan_count + s;
            int slot = launched % speedup;

            // join oldest slot if pool is full
            if ((launched - joined) >= speedup) 
            {
                int jslot = joined % speedup;
                if (alive[jslot]) 
                {
                    pthread_join(threads[jslot], NULL);
                    alive[jslot] = 0;
                }
                joined++;
                slot = launched % speedup;
            }

            targs[idx].target      = target;
            targs[idx].port        = ports[i];
            targs[idx].port_index  = idx;
            targs[idx].scan_type   = scan_types[s];
            targs[idx].scan_index  = s;
            targs[idx].result      = &results[i];
            targs[idx].mutex       = &mutex;

            if (pthread_create(&threads[slot], NULL, scan_port_thread, &targs[idx]) == 0) 
            {
                alive[slot] = 1;
                launched++;
            } 
            else 
                perror("pthread_create");
        }
    }

    // drain remaining 
    while (joined < launched) 
    {
        int slot = joined % speedup;
        if (alive[slot]) 
        {
            pthread_join(threads[slot], NULL);
            alive[slot] = 0;
        }
        joined++;
    }

    pthread_mutex_destroy(&mutex);

    //print results

    printf("\nScan Results:\n");
    printf("IP address: %s\n", ip);

    int col = 22; // wide enough for "XMAS(Open|Filtered)" + padding 

    printf("\nOpen ports:\n");
    printf("%-7s %-12s ", "Port", "Service");
    for (int s = 0; s < scan_count; s++)
        printf("%-*s", col, scanTypeStr[scan_types[s]]);
    printf("Conclusion\n");
    printf("--------------------------------------------------------------------\n");

    int any_open = 0;
    for (int i = 0; i < port_count; i++)
    {
        int is_open = 0;
        for (int s = 0; s < scan_count; s++)
        {
            if (strstr(results[i].results[s], "(Open)")) 
            { 
                is_open = 1;
                break; 
            }
        }
            
        if (!is_open) 
            continue;
        any_open = 1;
        printf("%-7d %-12s ", results[i].port, resolve_service(results[i].port));
        for (int s = 0; s < scan_count; s++)
            printf("%-*s", col, results[i].results[s]);
        printf("%s\n", derive_conclusion(&results[i], scan_types, scan_count));
    }
    if (!any_open) 
        printf("  (none)\n");

    printf("\nClosed/Filtered/Unfiltered ports:\n");
    printf("%-7s %-12s ", "Port", "Service");
    for (int s = 0; s < scan_count; s++)
        printf("%-*s", col, scanTypeStr[scan_types[s]]);
    printf("Conclusion\n");
    printf("--------------------------------------------------------------------\n");

    for (int i = 0; i < port_count; i++) 
    {
        int is_open = 0;
        for (int s = 0; s < scan_count; s++)
        {
            if (strstr(results[i].results[s], "(Open)")) 
            { 
                is_open = 1; 
                break;
            }
        }

        if (is_open)
            continue;
        printf("%-7d %-12s ", results[i].port, resolve_service(results[i].port));
        for (int s = 0; s < scan_count; s++)
            printf("%-*s", col, results[i].results[s]);
        printf("%s\n", derive_conclusion(&results[i], scan_types, scan_count));
    }

    free(threads); 
    free(alive); 
    free(targs); 
    free(results);
}
