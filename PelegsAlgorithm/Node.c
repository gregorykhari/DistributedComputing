#include <stdio.h>
#include "Node.h"

void PrintNodeInfo(struct Node nodeInfo)
{
    printf("<%s,%d> nodeInfo.myUID: %s\n",__FILE__,__LINE__,nodeInfo.myUID);
	printf("<%s,%d> nodeInfo.myHostName: %s\n",__FILE__,__LINE__,nodeInfo.myHostName);
	printf("<%s,%d> nodeInfo.myListeningPort: %s\n",__FILE__,__LINE__,nodeInfo.myListeningPort);

	int i;
	printf("<%s,%d> <<<<<NEIGHBOURS>>>>\n",__FILE__,__LINE__);
	printf("<%s,%d> UID\t\tHostname\t\tPort\n",__FILE__,__LINE__);
	for(i = 0 ; i < nodeInfo.numNeighbours; i++)
	{
		printf("<%s,%d> %s\t\t%s\t\t%s\n",__FILE__,__LINE__,nodeInfo.neighbourUIDs[i],nodeInfo.neighbourHostNames[i],nodeInfo.neighbourListeningPorts[i]);
	}
}