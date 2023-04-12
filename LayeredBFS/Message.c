#include "Message.h"

struct _Message CreateMessage(enum msgType messageType,uint32_t srcUID,uint32_t dstUID,uint32_t layer,uint32_t discovered)
{
    struct _Message tmpMessage;
    tmpMessage.msgT = messageType;
    tmpMessage.srcUID = srcUID;
    tmpMessage.dstUID = dstUID;
    tmpMessage.layer = layer;
    tmpMessage.discovered = discovered;

    return tmpMessage;
}