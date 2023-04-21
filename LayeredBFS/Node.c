#include <stdio.h>
#include "Node.h"

static const char* status_str[] = {"NON_DISTINGUISHED","DISTINGUISHED"};

void PrintNodeInfo(struct _Node node)
{

	printf("\n\n");
	printf("<%s,%s,%d>\t<<<<< NODE INFO >>>>>\n",__FILE__,__func__,__LINE__);
    printf("<%s,%s,%d>\tnode.myUID : %d\n",__FILE__,__func__,__LINE__,node.myUID);
	printf("<%s,%s,%d>\tnode.myHostName : %s\n",__FILE__,__func__,__LINE__,node.myHostName);
	printf("<%s,%s,%d>\tnode.myListeningPort : %d\n",__FILE__,__func__,__LINE__,node.myListeningPort);
    printf("<%s,%s,%d>\tnode.isDistinguished : %s\n",__FILE__,__func__,__LINE__,status_str[node.isDistinguished]);

	int i;
	printf("\n<%s,%s,%d>\t----- Neighbours -----\n",__FILE__,__func__,__LINE__);
	printf("<%s,%s,%d>\tUID\t\tHostname\t\t\tPort\t\tWeight\n",__FILE__,__func__,__LINE__);
	for(i = 0 ; i < node.numNeighbours; i++)
	{
		printf("<%s,%s,%d>\t%d\t\t%s\t\t%d\t\t%d\n",__FILE__,__func__,__LINE__,node.neighbourUIDs[i],node.neighbourHostNames[i],node.neighbourListeningPorts[i],node.neighbourEdgeWeights[i]);
	}
	printf("\n\n");
}

void PrintBFSInfo(struct _Node node)
{
	int i;
	printf("\n\n");
	printf("<%s,%s,%d>\t<<<<< NODE INFO >>>>>\n",__FILE__,__func__,__LINE__);
    printf("<%s,%s,%d>\tnode.myUID : %d\n",__FILE__,__func__,__LINE__,node.myUID);
	printf("<%s,%s,%d>\tnode.myHostName : %s\n",__FILE__,__func__,__LINE__,node.myHostName);
	printf("<%s,%s,%d>\tnode.myListeningPort : %d\n",__FILE__,__func__,__LINE__,node.myListeningPort);
    printf("<%s,%s,%d>\tnode.isDistinguished : %s\n",__FILE__,__func__,__LINE__,status_str[node.isDistinguished]);
	printf("<%s,%s,%d>\tnode.distanceFromDistinguishedNode : %d\n",__FILE__,__func__,__LINE__,node.myLayer);
	if(DISINGUISHED == node.isDistinguished)
	{
		printf("<%s,%s,%d>\tnode.parentUID : NULL\n",__FILE__,__func__,__LINE__);
	}
	else
	{
		printf("<%s,%s,%d>\tnode.parentUID : %d\n",__FILE__,__func__,__LINE__,node.parentUID);
	}
	printf("<%s,%s,%d>\tnode.childrenUIDs : {",__FILE__,__func__,__LINE__);
	for(i = 0; i < node.numChildren; i++)
	{
		printf("%d\t",node.childrenUIDs[i]);
	}
	printf(" }\n");
	if(DISINGUISHED == node.isDistinguished)
	{
		printf("<%s,%s,%d>\tnode.degree : %d\n",__FILE__,__func__,__LINE__,node.numChildren);
		printf("<%s,%s,%d>\tnode.maxDegree : %d\n",__FILE__,__func__,__LINE__,node.maxDegree);
	}
	else
	{
		printf("<%s,%s,%d>\tnode.degree : %d\n",__FILE__,__func__,__LINE__,(node.numChildren + 1));
	}
}
