#pragma once
#include "include.h"

#pragma pack(push, 1)
class Info
{
public:
    uint32_t addr;
    uint16_t port;
};

class Key
{
public:
    uint32_t addr;
    uint16_t port;

    Key() {}
    Key(uint32_t _addr, uint16_t _port) : addr(_addr), port(_port) {}

    bool operator < (const Key & ref) const
    {
        if(addr == ref.addr)
        {
            return port < ref.port;
        }
        else
        {
            return addr < ref.addr;
        }
    }

    void print_Key(void)
    {
        printf("%s\t",inet_ntoa(*reinterpret_cast<struct in_addr*>(&addr)));
        printf("%u\n", htons(port));

    }

};
#pragma pack(pop)

