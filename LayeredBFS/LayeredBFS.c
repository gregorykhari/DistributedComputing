#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <sys/socket.h>

#include "ConfigParser.h"
#include "Node.h"
#include "Message.h"
#include "MessageQueue.h"
#include "Connection.h"

#define BUFFER_SIZE 200

static struct _Node node;
static int discoveredNewNode = 0;
static int currentLayer = 0;

void PrintHelp();

void AddMessageToQueue(struct _Message);
void SendMessage(struct _Message);
void LayeredBFS();
void TerminateLayeredBFS();

void HandleMessages();
void HandleNewPhase(struct _Message);
void HandleACK(struct _Message);
void HandleNACK(struct _Message);
void HandleSearch(struct _Message);
void HandleTerminate(struct _Message);
void HandleInfo(struct _Message);

int getNeighbourIndex(int);
int receivedAllReplies();
void ResetReplies();
//void PrintReplies();

void LayeredBFS()
{
	int i;
	int currentLayer = 1;

	while(1)
	{
		discoveredNewNode = 0;

		printf("<%s,%s,%d>\tBuilding Layer %d Of LayeredBFS Spanning Tree!\n",__FILE__,__func__,__LINE__,currentLayer);
		ResetReplies();

		int msgType = (1 == currentLayer) ? SEARCH : NEW_PHASE;

		for(i = 0; i < node.numNeighbours; i++)
		{
			//create next layer 
			struct _Message send_msg = CreateMessage(msgType,node.myUID,node.neighbourUIDs[i],currentLayer,0,0);
			SendMessage(send_msg);
			node.outstandingMessageReplies = node.outstandingMessageReplies + 1;
		}

		//wait to receive ACKs\NACKs for NEW_PHASE from all children
		while(0 == receivedAllReplies())
		{
			sleep(1);
		}

		if(0 == discoveredNewNode)
		{
			printf("<%s,%s,%d>\tNo New Nodes Discovered In Network For Layer %d!\n",__FILE__,__func__,__LINE__,currentLayer);
			printf("<%s,%s,%d>\tTerminating LayeredBFS!\n",__FILE__,__func__,__LINE__,currentLayer);
    		TerminateLayeredBFS();
			break;
		}
		else
		{
			printf("<%s,%s,%d>\tAt Least 1 New Node Discovered In Network For Layer %d!\n",__FILE__,__func__,__LINE__,currentLayer);
			currentLayer = currentLayer + 1;
		}
	}
}

void TerminateLayeredBFS()
{
	int i = 0;
	for(i = 0; i < node.numChildren; i++)
	{
		//create next layer 
		struct _Message send_msg = CreateMessage(TERMINATE, node.myUID, node.childrenUIDs[i], currentLayer, 0,0);
		SendMessage(send_msg);
		node.outstandingMessageReplies = node.outstandingMessageReplies + 1;
	}
}

//accepts incoming connections from nodes and spawns a message handler thread
void AcceptConnections()
{
    node.mySocket = CreateSocket(node.myListeningPort);

	//listen for incoming connections on socket
	int error_code = listen(node.mySocket,1);
	if (error_code != 0)
	{
		printf("<%s,%s,%d>\tFailed to initiate listening on socket - error: %s!\n",__FILE__,__func__,__LINE__,strerror(errno));
		exit(1);
	}
	else
	{
		printf("<%s,%s,%d>\tSuccessfully initiated listening on socket %d!\n",__FILE__,__func__,__LINE__,node.mySocket);
	}

	while(1)
	{
		char recv_buffer[BUFFER_SIZE];

		//accept incoming connection request from client
		int node_socket = accept(node.mySocket,NULL,NULL);
		if (node_socket < 0)
		{
			printf("<%s,%s,%d>\tFailed to Accept Incoming Connection from client - error: %s!\n",__FILE__,__func__,__LINE__,strerror(errno));
			exit(1);
		}
		else
		{
			//do nothing
			printf("<%s,%s,%d>\tSuccessfully Accepted Incoming Connection with Neighbour !\n",__FILE__,__func__,__LINE__);
		}

		memset(recv_buffer,'\0',sizeof(recv_buffer));
		int error_code = recv(node_socket, &recv_buffer,sizeof(recv_buffer),0);
		struct _Message recv_msg = *((struct _Message *)recv_buffer);
		if (error_code < 0) 
		{                                                              
			printf("<%s,%s,%d>\tError While Receiving Message from Neighbour at Socket %d! Error %d : %s!\n",__FILE__,__func__,__LINE__,node_socket,error_code,strerror(errno));
			return;                                                              
		}
		else
		{
			printf("<%s,%s,%d>\tReceived %s(%d) Message from Neighbour with UID %d!\n",__FILE__,__func__,__LINE__,msgType_str[recv_msg.msgT],recv_msg.layer,recv_msg.srcUID);
			AddMessageToQueue(recv_msg);
		}
		close(node_socket);
    }
}

void AddMessageToQueue(struct _Message recv_msg)
{
	struct _MessageQueue* tmp = (struct _MessageQueue*) malloc(sizeof(struct _MessageQueue));
	tmp -> msg = recv_msg;
	tmp -> next = NULL;

	if(NULL == node.messageQueue)
	{
		node.messageQueue = tmp;
		node.messageQueueTailPtr = tmp;
	}
	else
	{
		node.messageQueueTailPtr -> next = tmp;
		node.messageQueueTailPtr = node.messageQueueTailPtr -> next;
	}
}

void SendMessage(struct _Message msg)
{

	int idx = getNeighbourIndex(msg.dstUID);

	int nodeSocket = ConnectToNode(msg.dstUID ,node.neighbourHostNames[idx],node.neighbourListeningPorts[idx]);
	
    int error_code = send(nodeSocket, &msg, sizeof(msg),0);
    if(error_code < 0)
    {
        printf("<%s,%s,%d>\tFailed to Send %s(%d) Message to Node With UID %d On Socket %d! Error_Code: %d\n", __FILE__, __func__, __LINE__, msgType_str[msg.msgT],msg.layer,msg.dstUID, nodeSocket,error_code);
        exit(1);
    }
    else
    {
        printf("<%s,%s,%d>\tSuccessfully Sent %s(%d) Message to Node With UID %d On Socket %d\n", __FILE__, __func__, __LINE__, msgType_str[msg.msgT],msg.layer,msg.dstUID, nodeSocket);
	}
}

void HandleMessages()
{
	struct _MessageQueue* messageQueuePtr = NULL;

    while(1)
    {
		messageQueuePtr = node.messageQueue;

		if(NULL == messageQueuePtr)
		{
			continue;
		}
		else
		{
			printf("<%s,%s,%d>\tProcessing %s(%d) Message Sent From Node With UID %d \n", __FILE__, __func__, __LINE__, msgType_str[messageQueuePtr->msg.msgT],messageQueuePtr->msg.layer,messageQueuePtr->msg.srcUID);

			switch(messageQueuePtr->msg.msgT)
			{
				case NEW_PHASE:
					HandleNewPhase(messageQueuePtr->msg);
					break;

				case SEARCH:
					HandleSearch(messageQueuePtr->msg);
					break;

				case ACK:
					HandleACK(messageQueuePtr->msg);
					break;

				case NACK:
					HandleNACK(messageQueuePtr->msg);
					break;

				case TERMINATE:
					HandleTerminate(messageQueuePtr->msg);
					break;

				case INFO:
					HandleInfo(messageQueuePtr->msg);
					break;

				default:
					break;
				
			}

			//save reference to first link
			struct _MessageQueue *temp = messageQueuePtr;
		
			//mark next to first link as first 
			node.messageQueue = node.messageQueue->next;
		
			//return the deleted link
			free(temp);
			temp = NULL;
		}
    }
}

void HandleNewPhase(struct _Message msg)
{
	discoveredNewNode = 0;

	//send search message to all neighbours except node we received new_phase from
	ResetReplies();

	if(msg.layer == node.myLayer + 1)
	{
		int i;
		for(i = 0; i < node.numNeighbours; i++)
		{
			if(node.neighbourUIDs[i] != node.parentUID)
			{
				struct _Message send_msg = CreateMessage(SEARCH,node.myUID,node.neighbourUIDs[i],msg.layer,0,0);
				SendMessage(send_msg);
				node.outstandingMessageReplies = node.outstandingMessageReplies + 1;
			}
		}
	}
	else
	{
		if(0 == node.numChildren)
		{
			struct _Message send_msg = CreateMessage(NACK,node.myUID,msg.srcUID,msg.layer,0,0);
			SendMessage(send_msg);
		}
		else
		{
			//rebroadcast message to all my children
			int i;
			for(i = 0; i < node.numChildren; i++)
			{
				struct _Message send_msg = CreateMessage(NEW_PHASE,node.myUID,node.childrenUIDs[i],msg.layer,0,0);
				SendMessage(send_msg);
				node.outstandingMessageReplies = node.outstandingMessageReplies + 1;
			}
		}
		
	}
}

void HandleSearch(struct _Message msg)
{
	if(1 == node.isMarked)
	{
		struct _Message send_msg = CreateMessage(NACK,node.myUID,msg.srcUID,msg.layer,0,0);
		SendMessage(send_msg);
	}
	else
	{
		//mark sender of search as parent and respond with ACK
		node.isMarked = 1;
		node.parentUID = msg.srcUID;
		node.myLayer = msg.layer;
		discoveredNewNode = 1;

		struct _Message send_msg = CreateMessage(ACK,node.myUID,msg.srcUID,msg.layer,1,0);
		SendMessage(send_msg);
	}
}

void HandleACK(struct _Message msg)
{
	//node.neighbourReplies[getNeighbourIndex(msg.srcUID)] = 1;
	//node.numMessagesReceived = node.numMessagesReceived + 1;
	node.outstandingMessageReplies = node.outstandingMessageReplies - 1;

	//put childUID in first available slot marked for children
	int i;
	for(i = 0; i < node.numNeighbours; i++)
	{
		if(-1 == node.childrenUIDs[i])
		{
			node.childrenUIDs[i] = msg.srcUID;
			node.numChildren = node.numChildren + 1;
			break;
		}
	}

	if(1 == msg.discovered)
	{
		discoveredNewNode = 1;
	}
	else
	{
		//do nothing
	}

	if((1 == receivedAllReplies()) && (NON_DISTINGUISHED == node.isDistinguished))
	{
		struct _Message send_msg = CreateMessage(NACK,node.myUID,node.parentUID,msg.layer,discoveredNewNode,0);
		SendMessage(send_msg);
	}
	else
	{
		//do nothing - wait for ACKs and NACKs 
	}
}

void HandleNACK(struct _Message msg)
{
	node.outstandingMessageReplies = node.outstandingMessageReplies - 1;

	if(1 == msg.discovered)
	{
		discoveredNewNode = 1;
	}
	else
	{
		//do nothing
	}

	if((1 == receivedAllReplies()) && (NON_DISTINGUISHED == node.isDistinguished))
	{
		struct _Message send_msg = CreateMessage(NACK,node.myUID,node.parentUID,msg.layer,discoveredNewNode,0);
		SendMessage(send_msg);
	}
	else
	{
		//do nothing - wait for ACKs and NACKs 
	}
}

void HandleTerminate(struct _Message msg)
{
	ResetReplies();

	int i;
	
	if(node.numChildren > 0)
	{
		for(i = 0; i < node.numChildren; i++)
		{
			struct _Message send_msg = CreateMessage(TERMINATE,node.myUID,node.childrenUIDs[i],msg.layer,0,0);
			SendMessage(send_msg);
			node.outstandingMessageReplies = node.outstandingMessageReplies + 1;
		}
	}
	else
	{
		struct _Message send_msg = CreateMessage(INFO,node.myUID,msg.srcUID,msg.layer,0,1);
		SendMessage(send_msg);
		node.terminationDetected = 1;
	}
}

void HandleInfo(struct _Message msg)
{
	node.outstandingMessageReplies = node.outstandingMessageReplies - 1;

	if(msg.degree > node.maxChildDegree)
	{
		node.maxChildDegree = msg.degree;
	}
	else
	{
		//do nothing
	}

	if(1 == receivedAllReplies())
	{
		int nodeDegree = node.numChildren + 1;
		int maxDegree = (nodeDegree > node.maxChildDegree) ? nodeDegree : node.maxChildDegree;
		
		if(NON_DISTINGUISHED == node.isDistinguished)
		{
			struct _Message send_msg = CreateMessage(INFO,node.myUID,node.parentUID,msg.layer,0,maxDegree);
			SendMessage(send_msg);
		}
		else
		{
			//do nothing
		}

		node.terminationDetected = 1;
	}
	else
	{
		//do nothing - wait for ACKs and NACKs 
	}
}

int receivedAllReplies()
{	
	return (0 == node.outstandingMessageReplies);
}

void ResetReplies()
{
	node.outstandingMessageReplies = 0;
}

int getNeighbourIndex(int uid)
{
	int i = 0;
	for(i = 0; i < node.numNeighbours; i++)
	{
		if(node.neighbourUIDs[i] == uid)
		{
			return i;
		}
	}
	return -1;
}

void InitNode(char* pathToConfig,char* myUID)
{
	node = Parse(pathToConfig,atoi(myUID));

	node.myLayer = 0;
	node.parentUID = -1;
	node.isMarked = 0;
	node.numChildren = 0;
	node.maxChildDegree = 0;
	node.messageQueue = NULL;
	node.messageQueueTailPtr = NULL;
	node.terminationDetected = 0;
	node.outstandingMessageReplies = 0;

	int i = 0;
	for(i = 0; i < node.numNeighbours; i++)
	{
		node.childrenUIDs[i] = -1;
	}

    PrintNodeInfo(node);
}

void PrintHelp()
{
	printf("Expected:\n\t\t./LayeredBFS.o -i <Path/To/Config/File> -u <UID> \n\n");
}


int main(int argc, char** argv)
{
    char* myUID;
	char* pathToConfig;

	int c = 1;
	while(c < argc)
	{
		if((0 == strcmp(argv[c],"-h")) || (0 == strcmp(argv[c],"-help")))
		{
			PrintHelp();
			exit(1);
		}
		else if(0 == strcmp(argv[c],"-i"))
		{
			c = c + 1;
			pathToConfig = argv[c];
		}
        else if(0 == strcmp(argv[c],"-u"))
		{
			c = c + 1;
			myUID = argv[c];
		}
		else
		{
			c = c + 1;
		}
	}

	InitNode(pathToConfig,myUID);

    pthread_t acceptConnection_tid;
	pthread_attr_t attr;                                                                                                                

	int error_code = pthread_attr_init(&attr);                                               
	if (error_code == -1) {                                                              
		printf("<%s,%s,%d>\tFailed to Initialize Attributes for Threads! Error_code: %d\n",__FILE__,__func__,__LINE__,error_code);
		exit(1);                                                                  
	}                                                                            

	int ds = PTHREAD_CREATE_DETACHED;                                                                      
	error_code = pthread_attr_setdetachstate(&attr, ds);                                
	if (error_code == -1) {                                                              
		printf("<%s,%s,%d>\tFailed to Set Detach State Attributes for Threads! Error_code: %d\n",__FILE__,__func__,__LINE__,error_code);
		exit(1);                                                                  
	}    

	//create thread for accepting incoming connections from clients
	error_code = pthread_create(&acceptConnection_tid,&attr,(void *)AcceptConnections,NULL);
	if(error_code != 0)
	{
		printf("<%s,%s,%d>\tFailed To Create AcceptConnections Thread! Error_code: %d\n",__FILE__,__func__,__LINE__,error_code);
		exit(1);
	}
	else
	{
		printf("<%s,%s,%d>\tSuccessfully Created Thread for AcceptConnections!\n",__FILE__,__func__,__LINE__);
		//do nothing
	}

	pthread_t HandleMessages_tid;
	
	//create thread for accepting incoming connections from clients
	error_code = pthread_create(&HandleMessages_tid,&attr,(void *)HandleMessages,NULL);
	if(error_code != 0)
	{
		printf("<%s,%s,%d>\tFailed To Create HandleMessages Thread! Error_code: %d\n",__FILE__,__func__,__LINE__,error_code);
		exit(1);
	}
	else
	{
		printf("<%s,%s,%d>\tSuccessfully Created Thread For HandleMessages!\n",__FILE__,__func__,__LINE__);
		//do nothing
	}

	sleep(5);

    if(node.isDistinguished == DISINGUISHED)
    {
        LayeredBFS();
    }
    else
    {
        //do nothing
    }

    //wait until LayeredBFS has terminated
	while(0 == node.terminationDetected)
	{
		sleep(1);		
	}

	PrintBFSInfo(node);

    return 0;
}