#include <stdio.h>
#include "Node.h"

static const char* status_str[] = {"NON_DISTINGUISHED","DISTINGUISHED"};

void PrintNodeInfo(struct _Node node)
{

	printf("\n\n");
	printf("<%s,%s,%d>\t<<<<< NODE INFO >>>>>\n",__FILE__,__func__,__LINE__);
    printf("<%s,%s,%d>\tnodeInfo.myUID: %d\n",__FILE__,__func__,__LINE__,node.myUID);
	printf("<%s,%s,%d>\tnodeInfo.myHostName: %s\n",__FILE__,__func__,__LINE__,node.myHostName);
	printf("<%s,%s,%d>\tnodeInfo.myListeningPort: %d\n",__FILE__,__func__,__LINE__,node.myListeningPort);
    printf("<%s,%s,%d>\tnodeInfo.isDistinguished: %s\n",__FILE__,__func__,__LINE__,status_str[node.isDistinguished]);

	int i;
	printf("\n<%s,%s,%d>\t----- Neighbours -----\n",__FILE__,__func__,__LINE__);
	printf("<%s,%s,%d>\tUID\t\tHostname\t\t\tPort\t\tWeight\n",__FILE__,__func__,__LINE__);
	for(i = 0 ; i < node.numNeighbours; i++)
	{
		printf("<%s,%s,%d>\t%d\t\t%s\t\t%d\t\t%d\n",__FILE__,__func__,__LINE__,node.neighbourUIDs[i],node.neighbourHostNames[i],node.neighbourListeningPorts[i],node.neighbourEdgeWeights[i]);
	}
	printf("\n\n");
}

void PrintBFSInfo(struct _Node nodeInfo)
{
	if(nodeInfo.isDistinguished == DISINGUISHED)
	{

	}
	else
	{

	}
}
