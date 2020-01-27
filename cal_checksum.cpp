#include "include.h"

void dump(unsigned char* buf, int size)
{
    int i;
    for (i = 0; i < size; i++) {
        if (i % 16 == 0)
            printf("\n");
        printf("%02x ", buf[i]);
    }
}


uint16_t calc(uint16_t * data, uint32_t data_len)
{
    uint32_t temp_checksum = 0;
    uint16_t checksum;

    uint32_t cnt, state;

    if(data_len % 2 == 0)
    {
        cnt = data_len / 2;
        state = 0;
    }
    else {
        cnt = (data_len / 2) + 1;
        state = 1;
    }


    for(int i = 0; i < cnt; i++)
    {
        if((i + 1) == cnt && state == 1)
            temp_checksum += ntohs((data[i] & 0x00ff));
        else
            temp_checksum += ntohs(data[i]);

    }

    temp_checksum = ((temp_checksum >> 16) & 0xffff) + temp_checksum & 0xffff;
    temp_checksum = ((temp_checksum >> 16) & 0xffff) + temp_checksum & 0xffff;

    checksum = temp_checksum;

    return checksum;
}

uint16_t cal_checksum_ip(uint8_t * data)
{
    struct iphdr * ip_header = reinterpret_cast<struct iphdr*>(data);
    uint16_t checksum;

    ip_header->check = 0;
    checksum = calc(reinterpret_cast<uint16_t *>(ip_header), (ip_header->ihl*4));

    ip_header->check = htons(checksum ^ 0xffff);

    return ip_header->check;
}

uint16_t cal_checksum_tcp(uint8_t * data)
{
    struct iphdr * ip_header = reinterpret_cast<struct iphdr*>(data);
    struct tcphdr * tcp_header = reinterpret_cast<struct tcphdr*>(data + ip_header->ihl*4);

    struct pseudohdr pseudo_header;

    memcpy(&pseudo_header.saddr, &ip_header->saddr, sizeof(uint32_t));
    memcpy(&pseudo_header.daddr, &ip_header->daddr, sizeof(uint32_t));
    pseudo_header.reserved = 0;
    pseudo_header.protocol = ip_header->protocol;
    pseudo_header.tcp_len = htons(ntohs(ip_header->tot_len) - (ip_header->ihl*4));

    uint16_t temp_checksum_pseudo, temp_checksum_tcp, checksum;
    uint32_t temp_checksum;

    tcp_header->check = 0;

    temp_checksum_pseudo = calc(reinterpret_cast<uint16_t *>(&pseudo_header), sizeof(pseudo_header));
    temp_checksum_tcp = calc(reinterpret_cast<uint16_t *>(tcp_header), ntohs(pseudo_header.tcp_len));

    temp_checksum = temp_checksum_pseudo + temp_checksum_tcp;

    temp_checksum = ((temp_checksum >> 16) & 0xffff) + temp_checksum & 0xffff;

    temp_checksum = ((temp_checksum >> 16) & 0xffff) + temp_checksum & 0xffff;

    checksum = temp_checksum;

    tcp_header->check = htons(checksum ^ 0xffff);

    return tcp_header->check;
}

void get_my_ip(char * dev, uint32_t * my_ip)
{
    /*        Get my IP Address      */
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    ifr.ifr_addr.sa_family = AF_INET;

    strncpy(ifr.ifr_name, dev, IFNAMSIZ-1);

    ioctl(fd, SIOCGIFADDR, &ifr);

    close(fd);
    memcpy(my_ip, &((((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr).s_addr), 4);
}

int tcp_connection(char * server_ip)
{
    int s, n;
    struct sockaddr_in server_addr;
    char _buf[BUF_LEN+1];



    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("can't create socket\n");
        exit(0);
    }

    bzero((char *)&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(0x0050);

    connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("connected\n");
    return s;
}
