#include <stdio.h>
#include "Node.h"

void PrintNodeInfo(struct Node nodeInfo)
{

	printf("\n\n");
    printf("<%s,%s,%d> nodeInfo.myUID: %s\n",__FILE__,__func__,__LINE__,nodeInfo.myUID);
	printf("<%s,%s,%d>  nodeInfo.myHostName: %s\n",__FILE__,__func__,__LINE__,nodeInfo.myHostName);
	printf("<%s,%s,%d>  nodeInfo.myListeningPort: %s\n",__FILE__,__func__,__LINE__,nodeInfo.myListeningPort);

	int i;
	printf("<%s,%s,%d>  <<<<<NEIGHBOURS>>>>\n",__FILE__,__func__,__LINE__);
	printf("<%s,%s,%d>  UID\t\tHostname\t\tPort\n",__FILE__,__func__,__LINE__);
	for(i = 0 ; i < nodeInfo.numNeighbours; i++)
	{
		printf("<%s,%s,%d> %s\t\t%s\t\t%s\n",__FILE__,__func__,__LINE__,nodeInfo.neighbourUIDs[i],nodeInfo.neighbourHostNames[i],nodeInfo.neighbourListeningPorts[i]);
	}
	printf("\n\n");
}

void PrintNodeBFSInfo(struct Node nodeInfo)
{
	int i;

	printf("<%s,%s,%d> Node Status: %d\n",__FILE__,__func__,__LINE__,nodeInfo.status);

	printf("<%s,%s,%d> Parent of Node with UID %s: \n",__FILE__,__func__,__LINE__,nodeInfo.parentUID);

	printf("<%s,%s,%d> Children of Node with UID %s: {",__FILE__,__func__,__LINE__,nodeInfo.myUID);
	for(i = 0; i < nodeInfo.numNeighbours; i++)
	{
		printf("%s,\t",nodeInfo.childrenUIDs[i]);
	}
	printf(" }");
}
