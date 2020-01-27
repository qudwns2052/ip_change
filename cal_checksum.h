#pragma once
#include "include.h"

#pragma pack(push, 1)

struct pseudohdr
{
    uint32_t saddr;
    uint32_t daddr;
    uint8_t reserved;
    uint8_t protocol;
    uint16_t tcp_len;
};

#pragma pack(pop)

uint16_t calc(uint16_t * data, uint32_t data_len);
uint16_t cal_checksum_ip(uint8_t * data);
uint16_t cal_checksum_tcp(uint8_t * data);
void dump(unsigned char* buf, int size);
void get_my_ip(char * dev, uint32_t * my_ip);
int tcp_connection(char * server_ip);
