#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h> 

enum msgType {NEW_PHASE, SEARCH, ACK, NACK};

struct _Message{
    uint32_t srcUID;
    uint32_t dstUID;
    uint32_t layer;
    uint32_t discovered;
    enum msgType msgT;
    
}__attribute__((packed));

struct _Message CreateMessage(uint32_t,uint32_t,uint32_t,uint32_t,enum msgType);

#endif