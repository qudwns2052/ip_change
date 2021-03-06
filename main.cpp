#include "include.h"

static unsigned char global_packet[10000];
static int global_ret = 0;
static uint32_t my_ip;
static uint32_t dst_ip;
static map<uint16_t, uint32_t> session;

/* returns packet id */
static u_int32_t print_pkt (struct nfq_data *tb)
{
    int id = 0;
    struct nfqnl_msg_packet_hdr *ph;
    struct nfqnl_msg_packet_hw *hwph;
    u_int32_t mark,ifi;
    int ret;
    unsigned char *data;

    ph = nfq_get_msg_packet_hdr(tb);
    if (ph)
    {
        id = ntohl(ph->packet_id);
    }
    ret = nfq_get_payload(tb, &data);


    //*****************************************************************//

    struct iphdr * ip_header = reinterpret_cast<struct iphdr*>(data);
    struct tcphdr * tcp_header = reinterpret_cast<struct tcphdr*>(data + ip_header->ihl*4);
    uint8_t * payload = data + (ip_header->ihl*4) + (tcp_header->th_off*4);



    if(ip_header->saddr == my_ip && ip_header->daddr != dst_ip)
    {
        if(session.find(tcp_header->th_sport) == session.end())
        {
            session[tcp_header->th_sport]= ip_header->daddr;
        }
        ip_header->daddr = dst_ip;
    }
    else if(ip_header->saddr == dst_ip && ip_header->daddr == my_ip)
    {
        if(session.find(tcp_header->th_dport) != session.end())
        {
            ip_header->saddr = session[tcp_header->th_dport];
        }
    }

    memset(global_packet, 0, 10000);
    memcpy(global_packet, data, ret);

    global_ret = ret;

    cal_checksum_ip(global_packet);
    cal_checksum_tcp(global_packet);



//    printf("------------------------------------------------------------\n");


    //*****************************************************************//




    return id;
}


static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
          struct nfq_data *nfa, void *data)
{
    u_int32_t id = print_pkt(nfa);
//    printf("entering callback\n");
    return nfq_set_verdict(qh, id, NF_ACCEPT, global_ret, global_packet);
}

int main(int argc, char **argv)
{
    //**************************************

    char * dst_ip_str = argv[1];
    char * dev= "eth0";

    get_my_ip(dev, &my_ip);
    dst_ip = inet_addr(dst_ip_str);

    system("iptables -F;iptables -F -t mangle");
    system("iptables -A OUTPUT -p udp --sport 53 -j ACCEPT");
    system("iptables -A OUTPUT -p udp --dport 53 -j ACCEPT");
    system("iptables -A PREROUTING -t mangle -p udp --sport 53 -j ACCEPT");
    system("iptables -A PREROUTING -t mangle -p udp --dport 53 -j ACCEPT");
    system("iptables -A OUTPUT -j NFQUEUE;iptables -A PREROUTING -t mangle -j NFQUEUE");


    //**************************************



    struct nfq_handle *h;
    struct nfq_q_handle *qh;
    struct nfnl_handle *nh;
    int fd;
    int rv;
    char buf[4096] __attribute__ ((aligned));

    printf("opening library handle\n");
    h = nfq_open();
    if (!h) {
        fprintf(stderr, "error during nfq_open()\n");
        exit(1);
    }

    printf("unbinding existing nf_queue handler for AF_INET (if any)\n");
    if (nfq_unbind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "error during nfq_unbind_pf()\n");
        exit(1);
    }

    printf("binding nfnetlink_queue as nf_queue handler for AF_INET\n");
    if (nfq_bind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "error during nfq_bind_pf()\n");
        exit(1);
    }

    printf("binding this socket to queue '0'\n");
    qh = nfq_create_queue(h,  0, &cb, NULL);
    if (!qh) {
        fprintf(stderr, "error during nfq_create_queue()\n");
        exit(1);
    }

    printf("setting copy_packet mode\n");
    if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
        fprintf(stderr, "can't set packet_copy mode\n");
        exit(1);
    }

    fd = nfq_fd(h);

    for (;;) {
        if ((rv = recv(fd, buf, sizeof(buf), 0)) >= 0) {
//            printf("pkt received\n");
            nfq_handle_packet(h, buf, rv);
            continue;
        }
        /* if your application is too slow to digest the packets that
         * are sent from kernel-space, the socket buffer that we use
         * to enqueue packets may fill up returning ENOBUFS. Depending
         * on your application, this error may be ignored. nfq_nlmsg_verdict_putPlease, see
         * the doxygen documentation of this library on how to improve
         * this situation.
         */
        if (rv < 0 && errno == ENOBUFS) {
            printf("losing packets!\n");
            continue;
        }
        perror("recv failed");
        break;
    }

    printf("unbinding from queue 0\n");
    nfq_destroy_queue(qh);

#ifdef INSANE
    /* normally, applications SHOULD NOT issue this command, since
     * it detaches other programs/sockets from AF_INET, too ! */
    printf("unbinding from AF_INET\n");
    nfq_unbind_pf(h, AF_INET);
#endif

    printf("closing library handle\n");
    nfq_close(h);

    exit(0);
}
