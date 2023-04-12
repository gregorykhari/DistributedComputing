#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include "Message.h"

struct _MessageQueue
{
    struct Message msg;
    struct _MessageQueue* next;
};

#endif