#pragma once

#include <pthread.h>


#if !defined MAX_NEIGHBOURS
#define MAX_NEIGHBOURS 100
#endif

enum LeaderStatus {LEADER, NON_LEADER, UNKNOWN};

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
    int maxRoundsInNeighbours[MAX_NEIGHBOURS];
    int maxUIDSeen;
    int maxDist;
    int currDistToNode;
    int currLeaderCount;
    pthread_t neighbourThreads[MAX_NEIGHBOURS];
    int marked; //for BFS - whether a node has received a 
    int parentUID; //the UID of the parent node
    int childrenUIDs[MAX_NEIGHBOURS]; // the UIDs of all children 
    enum LeaderStatus status;
};

void PrintNodeInfo(struct Node nodeInfo);

