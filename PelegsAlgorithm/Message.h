#pragma once

#include <stdint.h> 

enum msgType {CONNECTION, FLOOD, FLOOD_TERMINATE, SEARCH, ACK, NACK};

struct Message{
    uint32_t round;
    uint32_t srcUID;
    uint32_t dstUID;
    uint32_t currMaxUID;
    uint32_t currDist;
    uint32_t currMaxDist;
    enum msgType msgT;
    
}__attribute__((packed));