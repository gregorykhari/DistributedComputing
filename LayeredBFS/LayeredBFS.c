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
static struct _MessageQueue messageQueue;
static struct _MessageQueue* messageQueueTailPtr;

const char* msgType_str[] = {"NEW_PHASE", "SEARCH", "ACK", "NACK"};

void PrintHelp();
void AddMessageToQueue(struct _Message*);
void SendMessage(int, struct _Message);
void LayeredBFS();
void BuildLayer();
void TerminateLayeredBFS();

void LayeredBFS()
{
    int discoveredNewNode = 1;

    while(discoveredNewNode)
    {
        int i = 0;
        for(i = 0; i < node.numNeighbours; i++)
        {
            //create next layer 
            struct _Message msg = CreateMessage(NEW_PHASE,node.myUID,node.neighbourUIDs[i],node.myLayer,0);

            int nodeSocket = ConnectToNode(node.neighbourHostNames[i],node.neighbourListeningPorts[i]);
            SendMessage(nodeSocket,msg);
            CloseConnection(nodeSocket);
        }

        //wait to receive ACKs\NACKs for NEW_PHASE from all children
        while(1)
        {

        }
    }
   
    TerminateLayeredBFS();
}

void BuildLayer()
{

}

void TerminateLayeredBFS()
{

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
			AddMessageToQueue(&recv_msg);
		}
    }
}

void AddMessageToQueue(struct _Message* recv_msg)
{
	struct _MessageQueue* tmp = (struct _MessageQueue*) malloc(sizeof(struct _MessageQueue));
	tmp -> msg = recv_msg;
	tmp -> next = NULL;
	messageQueueTailPtr -> next = tmp;
	messageQueueTailPtr = messageQueueTailPtr -> next;
}

void SendMessage(int nodeSocket, struct _Message msg)
{
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
		messageQueuePtr = &messageQueue;

		switch(messageQueuePtr->msg.msgT)
		{
			case NEW_PHASE:
				break;

			case SEARCH:
				break;

			case ACK:
				break;

			case NACK:
				break;

			default:
				break;
            
        }
    }
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

    node = Parse(pathToConfig,atoi(myUID));

    PrintNodeInfo(node);

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

    if(node.isDistinguished == DISINGUISHED)
    {
        //LayeredBFS();
    }
    else
    {
        //do nothing
    }

    //wait until LayeredBFS has terminated

    return 0;
}