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

void LayeredBFS()
{
	int i;
	int currentLayer = 1;
	
	discoveredNewNode = 0;

	while(1)
	{
		node.numMessagesSent = node.numMessagesReceived = 0;

		int msgType = (1 == currentLayer) ? SEARCH : NEW_PHASE;

		for(i = 0; i < node.numNeighbours; i++)
		{
			//create next layer 
			struct _Message msg = CreateMessage(msgType,node.myUID,node.neighbourUIDs[i],currentLayer,0,0);
			SendMessage(msg);
			node.numMessagesSent = node.numMessagesSent + 1;
		}

		//wait to receive ACKs\NACKs for NEW_PHASE from all children
		while(node.numMessagesSent > node.numMessagesReceived)
		{
			sleep(1);
		}

		if(0 == discoveredNewNode)
		{
    		TerminateLayeredBFS();
			break;
		}
		else
		{
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
		struct _Message msg = CreateMessage(TERMINATE, node.myUID, node.childrenUIDs[i], currentLayer, 0,0);
		SendMessage(msg);
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
			printf("<%s,%s,%d>\tSuccessfully Accepted Incoming Connection with Neighbour at socket %d!\n",__FILE__,__func__,__LINE__,node_socket);
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
			printf("<%s,%s,%d>\tReceived %s Message from Neighbour with UID %d on Socket %d\n",__FILE__,__func__,__LINE__,msgType_str[recv_msg.msgT],recv_msg.srcUID,node_socket);
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
	}
	else
	{
		node.messageQueueTailPtr -> next = tmp;
		node.messageQueueTailPtr = node.messageQueueTailPtr -> next;
	}
}

void SendMessage(struct _Message msg)
{
	printf("<%s,%s,%d>\tSending %s Message to Node With UID %d\n", __FILE__, __func__, __LINE__, msgType_str[msg.msgT],msg.dstUID);

	int idx = getNeighbourIndex(msg.dstUID);

	int nodeSocket = ConnectToNode(msg.dstUID ,node.neighbourHostNames[idx],node.neighbourListeningPorts[idx]);
	
    int error_code = send(nodeSocket, &msg, sizeof(msg),0);
    if(error_code < 0)
    {
        printf("<%s,%s,%d>\tFailed to Send %s Message to Node With UID %d On Socket %d! Error_Code: %d\n", __FILE__, __func__, __LINE__, msgType_str[msg.msgT],msg.dstUID, nodeSocket,error_code);
        exit(1);
    }
    else
    {
        printf("<%s,%s,%d>\tSuccessfully Sent %s Message to Node With UID %d On Socket %d\n", __FILE__, __func__, __LINE__, msgType_str[msg.msgT],msg.dstUID, nodeSocket);
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
			printf("<%s,%s,%d>\tProcessing %s Message Sent From Node With UID %d \n", __FILE__, __func__, __LINE__, msgType_str[messageQueuePtr->msg.msgT],messageQueuePtr->msg.srcUID);

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

	if(msg.layer == node.myLayer + 1)
	{
		//send search message to all neighbours except node we received new_phase from
		node.numMessagesSent = node.numMessagesReceived = 0;

		int i;
		for(i = 0; i < node.numNeighbours; i++)
		{
			if(node.neighbourUIDs[i] != msg.srcUID)
			{
				struct _Message send_msg = CreateMessage(SEARCH,node.myUID,msg.srcUID,msg.layer,0,0);
				SendMessage(send_msg);
			}
		}

		//wait for ACKs and NACKs 
		while(node.numMessagesSent > node.numMessagesReceived)
		{
			sleep(1);
		}

		struct _Message send_msg = CreateMessage(ACK,node.myUID,msg.srcUID,msg.layer,discoveredNewNode,0);
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
		}
	}
}

void HandleSearch(struct _Message msg)
{
	if(0 == node.isMarked)
	{
		struct _Message msg = CreateMessage(NACK,node.myUID,msg.srcUID,msg.layer,0,0);
		SendMessage(msg);
	}
	else
	{
		//mark sender of search as parent and respond with ACK
		node.isMarked = 1;
		node.parentUID = msg.srcUID;
		node.myLayer = msg.layer;
		discoveredNewNode = 1;

		struct _Message send_msg = CreateMessage(ACK,node.myUID,msg.srcUID,msg.layer,discoveredNewNode,0);
		SendMessage(send_msg);
	}
}

void HandleACK(struct _Message msg)
{
	node.numMessagesReceived = node.numMessagesReceived + 1;

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
}

void HandleNACK(struct _Message msg)
{
	node.numMessagesReceived = node.numMessagesReceived + 1;
}

void HandleTerminate(struct _Message msg)
{
	node.numMessagesSent = node.numMessagesReceived = 0;

	int i;
	
	for(i = 0; i < node.numChildren; i++)
	{
		struct _Message send_msg = CreateMessage(TERMINATE,node.myUID,msg.srcUID,msg.layer,0,0);
		SendMessage(send_msg);
	}

	while(node.numMessagesSent > node.numMessagesReceived)
	{
		sleep(1);
	}

	int nodeDegree = node.numChildren + 1;
	int maxDegree = (nodeDegree > node.maxChildDegree) ? nodeDegree : node.maxChildDegree;

	struct _Message send_msg = CreateMessage(INFO,node.myUID,msg.srcUID,msg.layer,discoveredNewNode,maxDegree);
	SendMessage(send_msg);
}

void HandleInfo(struct _Message msg)
{
	node.numMessagesReceived = node.numMessagesReceived + 1;

	if(msg.degree > node.maxChildDegree)
	{
		node.maxChildDegree = msg.degree;
	}
	else
	{
		//do nothing
	}
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
	node.messageQueue = NULL;
	node.messageQueueTailPtr = NULL;
	node.terminationDetected = 0;

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