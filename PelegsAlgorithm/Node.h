#pragma once

#include <pthread.h>

#if !defined MAX_NEIGHBOURS
#define MAX_NEIGHBOURS 100
#endif

#if !defined CHAR_BUFFER_SIZE
#define CHAR_BUFFER_SIZE 100
#endif

enum LeaderStatus {LEADER, NON_LEADER, UNKNOWN};

struct Node
{
	char myUID[CHAR_BUFFER_SIZE];
    char myHostName[CHAR_BUFFER_SIZE];
    char myListeningPort[CHAR_BUFFER_SIZE];
    int mySocket;
    int numNeighbours;
    int numConnections;
	char neighbourHostNames[MAX_NEIGHBOURS][CHAR_BUFFER_SIZE];
	char neighbourListeningPorts[MAX_NEIGHBOURS][CHAR_BUFFER_SIZE];
    char neighbourUIDs[MAX_NEIGHBOURS][CHAR_BUFFER_SIZE];
    int neighbourSockets[MAX_NEIGHBOURS];  
    int maxRoundsInNeighbours[MAX_NEIGHBOURS];
    int maxUIDSeen;
    int maxDist;
    int currDistToNode;
    int currLeaderCount;
    int maxRound;
    pthread_t neighbourThreads[MAX_NEIGHBOURS];
    int marked; //for BFS - whether a node has received a 
    char parentUID[CHAR_BUFFER_SIZE]; //the UID of the parent node
    char childrenUIDs[MAX_NEIGHBOURS][CHAR_BUFFER_SIZE]; // the UIDs of all children 
    enum LeaderStatus status;
    int neighbourRepliedToSearch[MAX_NEIGHBOURS];
    int degree;
    int isBFSCompleted;
};

void PrintNodeInfo(struct Node nodeInfo);
void PrintNodeBFSInfo(struct Node nodeinfo);

