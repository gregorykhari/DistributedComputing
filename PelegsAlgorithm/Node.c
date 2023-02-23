#include <stdio.h>
#include "Node.h"

void PrintNodeInfo(struct Node nodeInfo)
{

	printf("\n\n");
    printf("<%s,%s,%d>\tnodeInfo.myUID: %s\n",__FILE__,__func__,__LINE__,nodeInfo.myUID);
	printf("<%s,%s,%d>\tnodeInfo.myHostName: %s\n",__FILE__,__func__,__LINE__,nodeInfo.myHostName);
	printf("<%s,%s,%d>\tnodeInfo.myListeningPort: %s\n",__FILE__,__func__,__LINE__,nodeInfo.myListeningPort);

	int i;
	printf("<%s,%s,%d>\t<<<<<NEIGHBOURS>>>>\n",__FILE__,__func__,__LINE__);
	printf("<%s,%s,%d>\tUID\t\tHostname\t\tPort\n",__FILE__,__func__,__LINE__);
	for(i = 0 ; i < nodeInfo.numNeighbours; i++)
	{
		printf("<%s,%s,%d>\t%s\t\t%s\t\t%s\n",__FILE__,__func__,__LINE__,nodeInfo.neighbourUIDs[i],nodeInfo.neighbourHostNames[i],nodeInfo.neighbourListeningPorts[i]);
	}
	printf("\n\n");
}

void PrintNodeBFSInfo(struct Node nodeInfo)
{
	int i;

	printf("<%s,%s,%d>\tNode Status: %d\n",__FILE__,__func__,__LINE__,nodeInfo.status);

	printf("<%s,%s,%d>\tParent of Node with UID %s: \n",__FILE__,__func__,__LINE__,nodeInfo.parentUID);

	printf("<%s,%s,%d>\tChildren of Node with UID %s: {",__FILE__,__func__,__LINE__,nodeInfo.myUID);
	for(i = 0; i < nodeInfo.numNeighbours; i++)
	{
		printf("%s,\t",nodeInfo.childrenUIDs[i]);
	}
	printf(" }");
}
