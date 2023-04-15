#ifndef NODE_H
#define NODE_H

#include "MessageQueue.h"

#if !defined MAX_NEIGHBOURS
#define MAX_NEIGHBOURS 100
#endif

#if !defined CHAR_BUFFER_SIZE
#define CHAR_BUFFER_SIZE 100
#endif

enum status
{
    NON_DISTINGUISHED,
    DISINGUISHED
};

struct _Node
{
    int myUID;
    char myHostName[CHAR_BUFFER_SIZE];
    int myListeningPort;
    int mySocket;
    int myLayer;
    enum status isDistinguished;

    int neighbourUIDs[MAX_NEIGHBOURS];
    int neighbourListeningPorts[MAX_NEIGHBOURS];
    char neighbourHostNames[MAX_NEIGHBOURS][CHAR_BUFFER_SIZE];
    int neighbourEdgeWeights[MAX_NEIGHBOURS];
    int numNeighbours;

    int parentUID;
    int childrenUIDs[MAX_NEIGHBOURS];
    int numChildren;
    int isMarked;

    struct _MessageQueue* messageQueue;
    struct _MessageQueue* messageQueueTailPtr;
    int numMessagesSent;
    int numMessagesReceived;

};

void PrintNodeInfo(struct _Node node);
void PrintBFSInfo(struct _Node nodeInfo);

#endif