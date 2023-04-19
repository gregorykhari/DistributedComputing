#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Message.h"

struct _Message CreateMessage(enum msgType messageType, uint32_t srcUID, uint32_t dstUID, uint32_t layer, uint32_t discovered, uint32_t degree)
{
    struct _Message tmpMessage;
    tmpMessage.msgT = messageType;
    tmpMessage.srcUID = srcUID;
    tmpMessage.dstUID = dstUID;
    tmpMessage.layer = layer;
    tmpMessage.discovered = discovered;
    tmpMessage.degree = degree;

    return tmpMessage;
}

void PrintMessage(struct _Message msg)
{
    printf("<%s,%s,%d> {%d,",__FILE__,__func__,__LINE__,msg.msgT);
    printf("%d,",msg.srcUID);
    printf("%d,",msg.dstUID);
    printf("%d,",msg.layer);
    printf("%d,",msg.discovered);
    printf("%d}\n",msg.degree);
}