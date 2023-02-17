#pragma once

#include <pthread.h>


#if !defined MAX_NEIGHBOURS
#define MAX_NEIGHBOURS 100
#endif

struct Node
{
	char myUID[10];
    char myHostName[10];
    char myListeningPort[10];
    int mySocket;
    int numNeighbours;
	char neighbourHostNames[MAX_NEIGHBOURS][10];
	char neighbourListeningPorts[MAX_NEIGHBOURS][10];
    char neighbourUIDs[MAX_NEIGHBOURS][10];
    int neighbourSockets[MAX_NEIGHBOURS];  
    pthread_t neighbourThreads[MAX_NEIGHBOURS];
};

void PrintNodeInfo(struct Node nodeInfo);

