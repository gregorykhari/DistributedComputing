#pragma once

#include <stdint.h> 
enum msgType {CONNECTION, FLOOD, FLOOD_TERMINATE};
struct Message{
    uint32_t round, srcUID, dstUID, currUID, currDist, currMaxDist;
    enum msgType msgT;
    
}__attribute__((packed));