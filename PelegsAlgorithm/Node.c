#include <stdio.h>
#include "Node.h"

static const char* status_str[] = {"LEADER", "NON_LEADER", "UNKNOWN"};

void PrintNodeInfo(struct Node nodeInfo)
{

	printf("\n\n");
	printf("<%s,%s,%d>\t<<<<< NODE INFO >>>>>\n",__FILE__,__func__,__LINE__);
    printf("<%s,%s,%d>\tnodeInfo.myUID: %s\n",__FILE__,__func__,__LINE__,nodeInfo.myUID);
	printf("<%s,%s,%d>\tnodeInfo.myHostName: %s\n",__FILE__,__func__,__LINE__,nodeInfo.myHostName);
	printf("<%s,%s,%d>\tnodeInfo.myListeningPort: %s\n",__FILE__,__func__,__LINE__,nodeInfo.myListeningPort);

	int i;

	printf("<%s,%s,%d>\t----- Neighbours -----\n",__FILE__,__func__,__LINE__);
	printf("<%s,%s,%d>\tUID\t\tHostname\tPort\n",__FILE__,__func__,__LINE__);
	for(i = 0 ; i < nodeInfo.numNeighbours; i++)
	{
		printf("<%s,%s,%d>\t%s\t\t%s\t\t%s\n",__FILE__,__func__,__LINE__,nodeInfo.neighbourUIDs[i],nodeInfo.neighbourHostNames[i],nodeInfo.neighbourListeningPorts[i]);
	}
	printf("\n\n");
}

void PrintNodeBFSInfo(struct Node nodeInfo)
{
	int i;

	printf("\n\n\n");
	printf("<%s,%s,%d>\t<<<<< NODE BFS INFO >>>>>\n",__FILE__,__func__,__LINE__);
	printf("<%s,%s,%d>\tNode Status: %s\n",__FILE__,__func__,__LINE__,status_str[nodeInfo.status]);

	if(LEADER == nodeInfo.status)
	{
		printf("<%s,%s,%d>\tParent of Node with UID %s : NULL\n",__FILE__,__func__,__LINE__,nodeInfo.myUID);
	}
	else
	{
		printf("<%s,%s,%d>\tParent of Node with UID %s : %s\n",__FILE__,__func__,__LINE__,nodeInfo.myUID,nodeInfo.parentUID);
	}

	printf("<%s,%s,%d>\tChildren of Node with UID %s: {",__FILE__,__func__,__LINE__,nodeInfo.myUID);
	for(i = 0; i < nodeInfo.numChildren; i++)
	{
		printf("%s\t",nodeInfo.childrenUIDs[i]);
	}

	printf(" }\n");

	if(LEADER == nodeInfo.status)
	{
		printf("<%s,%s,%d>\tDegree of Node with UID %s : %d\n",__FILE__,__func__,__LINE__,nodeInfo.myUID,nodeInfo.numChildren);
	}
	else
	{
		printf("<%s,%s,%d>\tDegree of Node with UID %s : %d\n",__FILE__,__func__,__LINE__,nodeInfo.myUID,(nodeInfo.numChildren + 1));
	}
}
