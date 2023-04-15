#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h> 

enum msgType {NEW_PHASE, SEARCH, ACK, NACK, TERMINATE, INFO};

struct _Message{
    uint32_t srcUID;
    uint32_t dstUID;
    uint32_t layer;
    uint32_t discovered;
    uint32_t degree;
    enum msgType msgT;
    
}__attribute__((packed));

struct _Message CreateMessage(enum msgType messageType, uint32_t srcUID, uint32_t dstUID, uint32_t layer, uint32_t discovered, uint32_t degree);

#endif