#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include<netdb.h>
#include <stdbool.h>

#include <errno.h>
#include <string.h>

#include <unistd.h>
#include <pthread.h>
#include "Node.h"

#include "ConfigParser.h"

#include "Message.h"

#define IP_ADDR INADDR_ANY
#define BUFFER_SIZE 200

static struct Node nodeInfo;
static bool isSync = false;
static const char* msgType_str[] = {"CONNECTION", "FLOOD", "FLOOD_TERMINATE", "SEARCH", "ACK", "NACK", "CLOSE"};
const char* status_str[] = {"LEADER", "NON_LEADER", "UNKNOWN"};

void PrintHelp();

void InitNode(char*,char*);
int getNeighbourIndex(char *node_uid);

int ValidatePort(int);
int ValidateIPAddress(char *);
int ResolveHostnameToIP(char* , char*);
int allConnectionsEstablished();
int CreateSocket(int port);
void ConnectToNeighbours();
void AcceptConnections();
void CloseConnections();
void CloseConnection(int idx);

int isSynchronized();
int getMaxRoundToStart();
struct Message CreateMessage(enum msgType msgT);
void Broadcast(struct Message msg);

void HandleMessages(void *);
void HandleSearchMessage(struct Message);
void HandleACKMessage(struct Message);
void HandleNACKMessage(struct Message);

int receivedFloodReplyFromAllNeighbours(int round);
void StartFlood();
void StartFloodTerminate();
void PelegsAlgorithm(struct Message msg);

void BFS();
int receivedBFSReplyFromAllNeighbours();

//boolean return for if all connections to neighbours established
int allConnectionsEstablished()
{
	return (nodeInfo.numConnections >= nodeInfo.numNeighbours);
}

//checks if port number is within valid range 
int ValidatePort(int port)
{
	if((port < 1024) || (port > 65535))
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

//checks if IP address is in expected format
int ValidateIPAddress(char * ip_addr)
{
	
	int count = 0;
	size_t j;
	for(j = 0; j < strlen(ip_addr);j++)
	{
		if (ip_addr[j] == '.')
		{
			count++;
		}
	}

	//should be 3 dots in an IPv4 address 
	if(3 != count)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

struct Message CreateMessage(enum msgType msgT)
{
	struct Message msg;
	msg.msgT = msgT;
	msg.round = nodeInfo.round;
	msg.srcUID = atoi(nodeInfo.myUID);
	
	return msg;
}

//boolean return if we received a reply to search message from all neighbours
int receivedBFSReplyFromAllNeighbours()
{
	int i, flag = 1;
	for(i = 0; i < nodeInfo.numNeighbours; i++)
	{
		if(nodeInfo.neighbourRepliedToSearch[i] == 0)
		{
			flag = 0;
		}
	}
	return flag;
}

//boolean return if we have received a reply from all neighbours for this round
int receivedFloodReplyFromAllNeighbours(int round)
{
	int i, flag = 1;
	for(i = 0; i < nodeInfo.numNeighbours; i++)
	{
		if(nodeInfo.maxRoundsInNeighbours[i] < round)
		{
			flag = 0;
		}
	}
	return flag;
}

//converts the hostname of a machine to the ip address
int ResolveHostnameToIP(char * hostname , char* ip)
{
	struct hostent *he;
	struct in_addr **addr_list;
	int i;
		
	if ((he = gethostbyname( hostname )) == NULL) 
	{
		// get the host info
		printf("<%s,%s,%d>\tFailed to Get Host Address List!",__FILE__,__func__,__LINE__);
		return 1;
	}

	addr_list = (struct in_addr **) he->h_addr_list;
	
	for(i = 0; addr_list[i] != NULL; i++) 
	{
		//Return the first one;
		strcpy(ip , inet_ntoa(*addr_list[i]) );
		return 0;
	}
	
	return 1;
}

//binds process (node) to a specific port and returns socket
int CreateSocket(int port)
{
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET; //specified for IPv4 connection
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = IP_ADDR;

	//create server socket
	int node_socket = socket(AF_INET,SOCK_STREAM,0);
	if (node_socket < 0)
	{
		printf("<%s,%s,%d>\tFailed to Create Socket! Error: %d!\n",__FILE__,__func__,__LINE__,errno,strerror(errno));
		return -1;
	}
	else
	{
		printf("<%s,%s,%d>\tSuccessfully Created Socket %d!\n",__FILE__,__func__,__LINE__,node_socket);
	}

	//assign server (IP) address to server socket
	int error_code = bind(node_socket,(struct sockaddr *) &server_addr,sizeof(server_addr));
	if (error_code != 0)
	{
		printf("<%s,%s,%d>\tFailed to Bind Socket! Error: %s, Error_code: %d!\n",__FILE__,__func__,__LINE__,strerror(errno),error_code);
		return -1;
	}
	else
	{
		printf("<%s,%s,%d>\tSuccessfully Binded Socket %d!\n",__FILE__,__func__,__LINE__,node_socket);
	}

    return node_socket;
}

//connects to randomly selected server with information returned by ConnectToServer
void Broadcast(struct Message msg)
{
	printf("<%s,%s,%d>\tBroadcasting Round %d %s Message to All Neighbours!\n", __FILE__, __func__, __LINE__, msg.round, msgType_str[msg.msgT]);

	int i;
	for( i = 0; i < nodeInfo.numNeighbours; i++){
		msg.dstUID = atoi(nodeInfo.neighbourUIDs[i]);
		printf("<%s,%s,%d>\tSending Round %d %s Message to Neighbour with UID %d on Socket %d!\n", __FILE__, __func__, __LINE__, msg.round, msgType_str[msg.msgT],msg.dstUID,nodeInfo.neighbourSockets[i]);
		
		int error_code = send(nodeInfo.neighbourSockets[i], &msg, sizeof(msg),0);
		if(error_code < 0)
		{
			printf("<%s,%s,%d>\tFailed to Send Round %d %s Message to Neighbour With UID %s On Socket %s! Error_Code: %d\n", __FILE__, __func__, __LINE__,msg.round, msgType_str[msg.msgT],nodeInfo.neighbourUIDs[i], nodeInfo.neighbourSockets[i],error_code);
			exit(1);
		}
		else
		{
			printf("<%s,%s,%d>\tSuccessfully Sent Round %d %s Message to Neighbour With UID %d On Socket %d\n", __FILE__, __func__, __LINE__, msg.round, msgType_str[msg.msgT],msg.dstUID, nodeInfo.neighbourSockets[i]);
		}
	}

	printf("<%s,%s,%d>\tSuccessfully Broadcasted ROUND %d %s Message to All Neighbours!\n", __FILE__, __func__, __LINE__, msg.round, msgType_str[msg.msgT]);
	nodeInfo.round = nodeInfo.round + 1;
}

//connects to nodes with smaller UIDs and spawns a message handler thread
void ConnectToNeighbours()
{
	char send_buffer[BUFFER_SIZE];
	int error_code;

	int node_socket_index;
	for(node_socket_index = 0; node_socket_index < nodeInfo.numNeighbours; node_socket_index++)
	{
		if(atoi(nodeInfo.myUID) < atoi(nodeInfo.neighbourUIDs[node_socket_index]))
		{
			//only try to connect to nodes which have a smaller UID than you 
			continue;
		}

		char ipAddr[100];
		char* hostname = nodeInfo.neighbourHostNames[node_socket_index];
		if(1 == ResolveHostnameToIP(hostname,ipAddr))
		{
			printf("<%s,%s,%d>\tFailed to Resolve Hostname %s to IPAddress!",__FILE__,__func__,__LINE__,hostname);
		}
		else
		{
			//do nothing 
		}

		int port = atoi(nodeInfo.neighbourListeningPorts[node_socket_index]);

		if(0 == ValidateIPAddress(ipAddr))
		{
			printf("<%s,%s,%d>\tIP Adress %s is not in expected format of X.X.X.X !\n",__FILE__,__func__,__LINE__,ipAddr);
			printf("<%s,%s,%d>\tFailed to Connect to Node!",__FILE__,__func__,__LINE__);
			exit(1);
		}

		if(0 == ValidatePort(port))
		{
			printf("<%s,%s,%d>\tPort %d not within valid range of 1024 to 65535!\n",__FILE__,__func__,__LINE__,port);
			printf("<%s,%s,%d>\tFailed to Connect to Node!",__FILE__,__func__,__LINE__);
			exit(1);
		}

		printf("<%s,%s,%d>\tAttempting to Connect to IP Adress %s on Port %d!\n",__FILE__,__func__,__LINE__,ipAddr,port);

		struct sockaddr_in server_addr;
		server_addr.sin_family = AF_INET; //specified for IPv4 connection
		server_addr.sin_port = htons(port);
		server_addr.sin_addr.s_addr = inet_addr(ipAddr);
		
		int connectionAttempts = 0;
		while(1)
		{
			//create a network socket for TCP/IP communication 
			nodeInfo.neighbourSockets[node_socket_index] = socket(AF_INET,SOCK_STREAM,0);
			if (nodeInfo.neighbourSockets[node_socket_index] < 0)
			{
				printf("<%s,%s,%d>\tFailed to create socket! Error_code: %d .Error: %s!\n",__FILE__,__func__,__LINE__,error_code,strerror(errno));
			}
			
			error_code = connect(nodeInfo.neighbourSockets[node_socket_index],(struct sockaddr *)&server_addr,sizeof(server_addr));
			if (error_code != 0)
			{
				close(nodeInfo.neighbourSockets[node_socket_index]);
			}
			else
			{
				printf("<%s,%s,%d>\tSuccessfully Established Connection to Neighbour with UID %s At IPAddress %s on Port %d With Socket %d!\n",__FILE__,__func__,__LINE__,nodeInfo.neighbourUIDs[node_socket_index],ipAddr,port,nodeInfo.neighbourSockets[node_socket_index]);
				break;
			}
		}
			
		//send message to other node to let it know which socket to assign to
		struct Message msg = CreateMessage(CONNECTION);
		error_code = send(nodeInfo.neighbourSockets[node_socket_index], &msg, sizeof(msg),0);
		if (error_code < 0)
		{
			printf("<%s,%s,%d>\tError Sending Message (%s) to Server on Socket %d - error: %s!\n",__FILE__,__func__,__LINE__,send_buffer,nodeInfo.neighbourSockets[node_socket_index],strerror(errno));
		}
		else
		{
			//do nothing
		}

		//create a variable to pass network socket to thread args
		int* nsi = malloc(sizeof(int));
		if(NULL == nsi)
		{
			printf("<%s,%s,%d>\tFailed to malloc for nsi!\n",__FILE__,__func__,__LINE__);
		}
		else
		{
				//do nothing
		}

		*nsi = node_socket_index;

		pthread_attr_t attr;                                                                                                                

		error_code = pthread_attr_init(&attr);                                               
		if (error_code == -1) {                                                              
			printf("<%s,%s,%d>\tFailed to create HandleMessages thread for Client at socket %d - error: %d\n",__FILE__,__func__,__LINE__,error_code,nodeInfo.neighbourSockets[node_socket_index]);
			exit(1);                                                                  
		}                                                                            

		int ds = PTHREAD_CREATE_DETACHED;                                                                      
		error_code = pthread_attr_setdetachstate(&attr, ds);                                
		if (error_code == -1) {                                                              
			printf("<%s,%s,%d>\tFailed to create HandleMessages thread for Client at socket %d - error: %d\n",__FILE__,__func__,__LINE__,error_code,nodeInfo.neighbourSockets[node_socket_index]);
			exit(1);                                                                  
		}     

		//create thread for receiving data from servers
		error_code = pthread_create(&nodeInfo.neighbourThreads[node_socket_index],&attr,(void *)HandleMessages,nsi);
		if(error_code != 0)
		{
			printf("<%s,%s,%d>\tFailed to create HandleServer thread for Client at socket %d - error: %d\n",__FILE__,__func__,__LINE__,error_code,nodeInfo.neighbourSockets[node_socket_index]);
			exit(1);
		}
		else
		{
			//do nothing
		}

		//increment the number of connections
		nodeInfo.numConnections = nodeInfo.numConnections + 1;
	}
}

//accepts incoming connections from nodes and spawns a message handler thread
void AcceptConnections()
{

	//listen for incoming connections on socket
	int error_code = listen(nodeInfo.mySocket,1);
	if (error_code != 0)
	{
		printf("<%s,%s,%d>\tFailed to initiate listening on socket - error: %s!\n",__FILE__,__func__,__LINE__,strerror(errno));
		exit(1);
	}
	else
	{
		printf("<%s,%s,%d>\tSuccessfully initiated listening on socket %d!\n",__FILE__,__func__,__LINE__,nodeInfo.mySocket);
	}

	while(1)
	{
		char recv_buffer[BUFFER_SIZE];

		//accept incoming connection request from client
		int node_socket = accept(nodeInfo.mySocket,NULL,NULL);
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
		struct Message recv_msg = *((struct Message *)recv_buffer);
		if (error_code < 0) 
		{                                                              
			printf("<%s,%s,%d>\tError While Receiving Message from Neighbour at Socket %d! Error %d : %s!\n",__FILE__,__func__,__LINE__,node_socket,error_code,strerror(errno));
			return;                                                              
		}
		else
		{
			printf("<%s,%s,%d>\tReceived %s Message from Neighbour with UID %d on Socket %d\n",__FILE__,__func__,__LINE__,msgType_str[recv_msg.msgT],recv_msg.srcUID,node_socket);
		}

		//determine index of node based on srcUID of CONNECTION message just received
		char node_uid[BUFFER_SIZE];
		sprintf(node_uid, "%d", recv_msg.srcUID);
		int neighbourIndex = getNeighbourIndex(node_uid);
		nodeInfo.neighbourSockets[neighbourIndex] = node_socket;

		printf("<%s,%s,%d>\tSuccessfully Established Connection With Neighbour %s on Socket %d\n",__FILE__,__func__,__LINE__,node_uid,nodeInfo.neighbourSockets[neighbourIndex]);

		//create a variable to pass network socket to thread args
		int* ni = malloc(sizeof(int));
		if(NULL == ni)
		{
			printf("<%s,%s,%d>\tFailed to malloc for nsi for socket %d!\n",__FILE__,__func__,__LINE__,node_socket);
			exit(1);
		}
		else
		{
			//do nothing
			*ni = neighbourIndex;
		}

		

		pthread_attr_t attr;                                                                                                                

		error_code = pthread_attr_init(&attr);                                               
		if (error_code == -1) {                                                              
			printf("<%s,%s,%d>\tFailed to create HandleMessages thread for Client at socket %d! Error_code: %d, Error: %s \n",__FILE__,__func__,__LINE__,error_code,nodeInfo.neighbourSockets[neighbourIndex],strerror(errno));
			exit(1);                                                                  
		}                                                                            

		int ds = PTHREAD_CREATE_DETACHED;                                                                      
		error_code = pthread_attr_setdetachstate(&attr, ds);                                
		if (error_code == -1) {                                                              
			printf("<%s,%s,%d>\tFailed to create HandleMessages thread for Client at socket %d! Error_code: %d, Error: %s \n",__FILE__,__func__,__LINE__,error_code,nodeInfo.neighbourSockets[neighbourIndex],strerror(errno));
			exit(1);                                                                  
		}     

		//create thread for receiving data from servers
		error_code = pthread_create(&nodeInfo.neighbourThreads[neighbourIndex],&attr,(void *)HandleMessages,ni);
		if(error_code != 0)
		{
			printf("<%s,%s,%d>\tFailed to create HandleServer thread for Client at socket %d! Error_code: %d, Error: %s \n",__FILE__,__func__,__LINE__,error_code,nodeInfo.neighbourSockets[neighbourIndex],strerror(errno));
			exit(1);
		}
		else
		{
			//do nothing
		}

		nodeInfo.numConnections = nodeInfo.numConnections + 1;
	}
}

void HandleMessages(void *ni)
{
	int neighbourIndex = *((int*)ni);

	char recv_buffer[BUFFER_SIZE];

	memset(recv_buffer,'\0',sizeof(recv_buffer));
	int error_code = recv(nodeInfo.neighbourSockets[neighbourIndex], &recv_buffer,sizeof(recv_buffer),0);
	struct Message recv_msg = *((struct Message *)recv_buffer);
	if (error_code < 0) 
	{                                                              
		printf("<%s,%s,%d>\tFailed to receive message!\t Error_code: %d\n",__FILE__,__func__,__LINE__,error_code);
		return;                                                                
	}
	else
	{
		if (recv_msg.msgT != CONNECTION)
			printf("<%s,%s,%d>\tSuccessfully Received Round %d %s Message from Neighbour with UID %d on Socket %d\n",__FILE__,__func__,__LINE__,recv_msg.round,msgType_str[recv_msg.msgT],recv_msg.srcUID,nodeInfo.neighbourSockets[neighbourIndex]);
	}

	while(1)
	{
		//determine round for which message was sent
		if(recv_msg.round > nodeInfo.maxRoundsInNeighbours[neighbourIndex])
		{
			printf("<%s,%s,%d>\tUpdating nodeInfo.maxRoundsInNeighbours[%d]= %d\n",__FILE__,__func__,__LINE__,neighbourIndex,recv_msg.round);
			nodeInfo.maxRoundsInNeighbours[neighbourIndex] = recv_msg.round;
		}

		//check if received a 
		while(0 == isSynchronized());
		
		switch (recv_msg.msgT)
		{
			case CONNECTION:
				return;
				break;
			
			case FLOOD:
				PelegsAlgorithm(recv_msg);
				break;

			case FLOOD_TERMINATE:
				if(UNKNOWN == nodeInfo.status)
				{
					nodeInfo.status = NON_LEADER;
					struct Message reply_msg = CreateMessage(FLOOD_TERMINATE);
					reply_msg.currMaxUID = nodeInfo.maxUIDSeen;
					reply_msg.currDist = nodeInfo.currDistToNode;
					reply_msg.currMaxDist = nodeInfo.maxDist;
					Broadcast(reply_msg);
				}
				break;

			case SEARCH:
				HandleSearchMessage(recv_msg);
				break;

			case ACK:
				HandleACKMessage(recv_msg);
				break;

			case NACK:
				HandleNACKMessage(recv_msg);
				break;
			
			case CLOSE:
				CloseConnection(neighbourIndex);
				return;
				break;

			default:
				printf("<%s,%s, %d>: Message Type:\t  Type:%d Src: %d \t Received:%d\n\n", __FILE__, __func__, __LINE__, recv_msg.msgT, recv_msg.srcUID, recv_msg.dstUID);
				break;
		}

		memset(recv_buffer,'\0',sizeof(recv_buffer));
		int error_code = recv(nodeInfo.neighbourSockets[neighbourIndex], &recv_buffer,sizeof(recv_buffer),0);
		recv_msg = *((struct Message *)recv_buffer);
		if (error_code < 0) 
		{                                                              
			printf("<%s,%s,%d>\tFailed To Receive Message From Neighbour with UID %s On Socket %d!\n",__FILE__,__func__,__LINE__,nodeInfo.neighbourUIDs[neighbourIndex],nodeInfo.neighbourSockets[neighbourIndex]);
			exit(1);                                                                  
		}
		else
		{
			if (recv_msg.msgT != CONNECTION)
				printf("<%s,%s,%d>\tSuccessfully Received Round %d %s Message from Neighbour with UID %d on Socket %d\n",__FILE__,__func__,__LINE__,recv_msg.round,msgType_str[recv_msg.msgT],recv_msg.srcUID,nodeInfo.neighbourSockets[neighbourIndex]);
		}
	}
}

void HandleSearchMessage(struct Message msg)
{

	char srcUID[BUFFER_SIZE];
	sprintf(srcUID,"%d",msg.srcUID);

	if(1 == nodeInfo.marked)
	{
		printf("<%s,%s,%d>\tAlready Marked Neighbour with UID %s As Parent!",__FILE__,__func__,__LINE__,nodeInfo.parentUID);

		//determine socket to send NACK message back to base on recv_msg srcUID
		int socketIndex = getNeighbourIndex(srcUID);

		//check if we already sent search messages to all neighbours
		struct Message msg = CreateMessage(NACK);
		msg.dstUID = atoi(nodeInfo.neighbourUIDs[socketIndex]);
		int error_code = send(nodeInfo.neighbourSockets[socketIndex],&msg,sizeof(msg),0);
		if(error_code < 0)
		{
			printf("<%s,%s,%d>\tFailed to Send NACK Message to Neighbour with UID %s on Socket %d!",__FILE__,__func__,__LINE__,nodeInfo.neighbourUIDs[socketIndex],nodeInfo.neighbourSockets[socketIndex]);
			exit(1);
		}
		else
		{
			//do nothing- message was sent successfully
			printf("<%s,%s,%d>\tSuccessfully Sent NACK Message to Neighbour with UID %s on Socket %d!",__FILE__,__func__,__LINE__,nodeInfo.neighbourUIDs[socketIndex],nodeInfo.neighbourSockets[socketIndex]);
		}
	}
	else
	{
		
		printf("<%s,%s,%d>\tMarking Neighbour with UID %s As Parent!",__FILE__,__func__,__LINE__,nodeInfo.parentUID);

		//mark the first node we received a search message from as parent
		nodeInfo.marked = 1;

		int index = getNeighbourIndex(srcUID);

		nodeInfo.neighbourRepliedToSearch[index] = 1;

		//save node we received SEARCH from as parent
		char parentUID[10];
		sprintf(parentUID,"%d",msg.srcUID);
		strcpy(nodeInfo.parentUID,parentUID);

		//send search message to all children
		BFS();

		while(0 == receivedBFSReplyFromAllNeighbours())
		{
			sleep(1);
		}

		//check if we already sent search messages to all neighbours
		struct Message msg = CreateMessage(ACK);
		msg.dstUID = atoi(nodeInfo.neighbourUIDs[index]);
		int error_code = send(nodeInfo.neighbourSockets[index],&msg,sizeof(msg),0);
		if(error_code < 0)
		{
			printf("<%s,%s,%d>\tFailed to Send ACK Message to Neighbour with UID %s on Socket %d!",__FILE__,__func__,__LINE__,nodeInfo.neighbourUIDs[index],nodeInfo.neighbourSockets[index]);
			exit(1);
		}
		else
		{
			//do nothing- message was sent successfully
			printf("<%s,%s,%d>\tSuccessfully Sent ACK Message to Neighbour with UID %s on Socket %d!",__FILE__,__func__,__LINE__,nodeInfo.neighbourUIDs[index],nodeInfo.neighbourSockets[index]);
		}


		
	}
}

void HandleACKMessage(struct Message msg)
{
	char srcUID[10];
	sprintf(srcUID,"%d",msg.srcUID);

	int neighbourIndex = getNeighbourIndex(srcUID);

	//set that neighbour has replied
	nodeInfo.neighbourRepliedToSearch[neighbourIndex] = 1;
	
	//put childUID in first available slot marked for children
	int i;
	for(i = 0; i < nodeInfo.numNeighbours; i++)
	{
		if(0 == strcmp(nodeInfo.childrenUIDs[i],"\0"))
		{
			strcpy(nodeInfo.childrenUIDs[i],srcUID);
			break;
		}
	}

	// struct Message send_msg = CreateMessage(ACK);
	// send_msg.round = nodeInfo.round;
	// msg.dstUID = msg.srcUID;

	// //if all neighbours have now replied, send ACK up to parent
	// if(1 == receivedBFSReplyFromAllNeighbours())
	// {
	// 	//perform converge cast and finally send ACK to parent
	// 	send_msg.round = nodeInfo.round;
	// 	int error_code = send(nodeInfo.neighbourSockets[i],&send_msg,sizeof(send_msg),0);
	// 	if(error_code < 0)
	// 	{
	// 		printf("<%s,%s,%d>\tFailed to Send Round %d %s Message to Neighbour with UID %s on Socket %d!",__FILE__,__func__,__LINE__,send_msg.round,msgType_str[send_msg.msgT],nodeInfo.neighbourUIDs[i],nodeInfo.neighbourSockets[i]);
	// 		exit(1);
	// 	}
	// 	else
	// 	{
	// 		//do nothing- message was sent successfully
	// 		printf("<%s,%s,%d>\tSuccessfully Sent Round %d %s Message to Neighbour with UID %s on Socket %d!",__FILE__,__func__,__LINE__,send_msg.round,msgType_str[send_msg.msgT],nodeInfo.neighbourUIDs[i],nodeInfo.neighbourSockets[i]);
	// 	}
	// }
	// else
	// {
	// 	//do nothing - still need more replies to proceed
	// }

	//nodeInfo.round = nodeInfo.round + 1;
	
}

void HandleNACKMessage(struct Message msg)
{
	int i;

	char srcUID[10];
	sprintf(srcUID,"%d",msg.srcUID);

	int neighbourIndex = getNeighbourIndex(srcUID);

	//set that neighbour has replied
	nodeInfo.neighbourRepliedToSearch[neighbourIndex] = 1;
	
	// //if all neighbours have now replied, send ACK up to parent
	// if(1 == receivedBFSReplyFromAllNeighbours())
	// {
	// 	//perform converge cast and finally send ACK to parent
	// 	struct Message send_msg = CreateMessage(NACK);
	// 	send_msg.round = nodeInfo.round;
	// 	msg.dstUID = msg.srcUID;
	// 	int error_code = send(nodeInfo.neighbourSockets[i],&send_msg,sizeof(send_msg),0);
	// 	if(error_code < 0)
	// 	{
	// 		printf("<%s,%s,%d>\tFailed to Send Round %d %s Message to Neighbour with UID %s on Socket %d!",__FILE__,__func__,__LINE__,send_msg.round,msgType_str[send_msg.msgT],nodeInfo.neighbourUIDs[i],nodeInfo.neighbourSockets[i]);
	// 		exit(1);
	// 	}
	// 	else
	// 	{
	// 		//do nothing- message was sent successfully
	// 		printf("<%s,%s,%d>\tSuccessfully Sent Round %d %s Message to Neighbour with UID %s on Socket %d!",__FILE__,__func__,__LINE__,send_msg.round,msgType_str[send_msg.msgT],nodeInfo.neighbourUIDs[i],nodeInfo.neighbourSockets[i]);
	// 	}
	// }
	// else
	// {
	// 	//do nothing - still need more replies to proceed
	// }

	// nodeInfo.round = nodeInfo.round + 1;

}

int isSynchronized()
{
	int currMin = nodeInfo.maxRoundsInNeighbours[0];
	int currMax = nodeInfo.maxRoundsInNeighbours[0];

	int i;
	for (i = 0 ;i < nodeInfo.numNeighbours; i++)
	{

		if (nodeInfo.maxRoundsInNeighbours[i] == -1)
		{
			return 0;
		}
		if (nodeInfo.maxRoundsInNeighbours[i] < currMin)
		{
			currMin = nodeInfo.maxRoundsInNeighbours[i];
		}
		
		if(nodeInfo.maxRoundsInNeighbours[i] > currMax)
		{
			currMax = nodeInfo.maxRoundsInNeighbours[i];
		}
	}

	if ((currMax - currMin) >= 2){
		return 0;
	}
	else
	{
		return 1;
	}
}

int getMaxRoundToStart(){
	int maxVal = 0;
	int i;
	for(i = 0; i < nodeInfo.numNeighbours; i++){
		if(nodeInfo.maxRoundsInNeighbours[i] > maxVal){
			maxVal = nodeInfo.maxRoundsInNeighbours[i];
		}
	}
	return maxVal + 1;
}

int getNeighbourIndex(char *node_uid){
	int i, neighbourIndex = -1;
	for(i = 0; i < nodeInfo.numNeighbours; i++)
	{
		if(0 == strcmp(nodeInfo.neighbourUIDs[i],node_uid))
		{
			neighbourIndex = i;
			break;
		}
	}
	return neighbourIndex;
}

void CloseConnections()
{
	int i = 0;
	//struct Message closeConnectionMsg = CreateMessage(CLOSE);
	//closeConnectionMsg.round = nodeInfo.round + 1;
	//Broadcast(closeConnectionMsg);
	for(i = 0; i < nodeInfo.numNeighbours; i++)
	{

		close(nodeInfo.neighbourSockets[i]);
		printf("<%s,%s,%d>\tSuccessfully Closed Connections to Neighbour with UID %s",__FILE__,__func__,__LINE__,nodeInfo.neighbourUIDs[i]);
	}
}

void CloseConnection(int index)
{
	close(nodeInfo.neighbourSockets[index]);
	printf("<%s,%s,%d>\tSuccessfully Closed Connections to Neighbour with UID %s",__FILE__,__func__,__LINE__,nodeInfo.neighbourUIDs[index]);
}

void PelegsAlgorithm(struct Message msg)
{
	printf("<%s,%s,%d>\tNode.maxUIDSeen: %d, Node.Round: %d, Msg.currMaxUID: %d, Msg.Round: %d!\n", __FILE__, __func__, __LINE__,nodeInfo.maxUIDSeen,nodeInfo.round, msg.currMaxUID,msg.round);

	if((nodeInfo.maxUIDSeen == msg.currMaxUID) && (msg.round > nodeInfo.currMaxUIDRound))
	{
		printf("<%s,%s,%d>\tNode.maxUIDSeen: %d, Node.Round: %d, Msg.currMaxUID: %d, Msg.Round: %d!\n", __FILE__, __func__, __LINE__,nodeInfo.maxUIDSeen,nodeInfo.round, msg.currMaxUID,msg.round);

		nodeInfo.currLeaderRoundCount++;
		nodeInfo.currMaxUIDRound = msg.round;
	}
	else
	{
		//do nothing
	}
	
	if((nodeInfo.currLeaderRoundCount > 3) && (nodeInfo.maxUIDSeen == msg.currMaxUID) && (nodeInfo.maxUIDSeen == atoi(nodeInfo.myUID)))
	{
		//Start a flood terminate message!
		printf("<%s,%s,%d>\tReceived 3 Rounds of Messages With Same Max UID %d!\n", __FILE__, __func__, __LINE__, msg.currMaxUID);
		printf("<%s,%s,%d>\tStart the FLOOD TERMINATION!\n", __FILE__, __func__, __LINE__);
		nodeInfo.status = LEADER;
		StartFloodTerminate();
	}
	else
	{
		if(nodeInfo.maxUIDSeen < msg.currMaxUID)
		{
			printf("<%s,%s,%d>\tReceived Message with Larger UID %d Than Current Max %d\n", __FILE__, __func__, __LINE__,msg.currMaxUID,nodeInfo.maxUIDSeen);
			printf("<%s,%s,%d>\tUpdating Current Max UID!\n", __FILE__, __func__, __LINE__);

			nodeInfo.maxUIDSeen = msg.currMaxUID;
			nodeInfo.currDistToNode = msg.currDist + 1;
			nodeInfo.maxDist = nodeInfo.currDistToNode;
			nodeInfo.currLeaderRoundCount = 1;
		}
		else if(nodeInfo.maxUIDSeen == msg.currMaxUID)
		{
			printf("<%s,%s,%d>\tReceived Message with the Same MAX UID %d\n", __FILE__, __func__, __LINE__,msg.currMaxUID);
			printf("<%s,%s,%d>\tSetting the MAX Distance\n", __FILE__, __func__, __LINE__);

			if(msg.currMaxDist > nodeInfo.maxDist)
			{
				nodeInfo.currLeaderRoundCount = 1;
				nodeInfo.maxDist = msg.currMaxDist;
			}
				
			//printf("<%s,%s,%d>\tRECV Same UID\t maxDistance: %d\n", __FILE__, __func__, __LINE__, nodeInfo.maxDist);
		}

		// if(nodeInfo.round < msg.round)
		// {
		// 	struct Message reply_msg = CreateMessage(FLOOD);
		// 	reply_msg.round = msg.round;
		// 	Broadcast(reply_msg);
		// }
		// else
		// {
		// 	//do nothing
		// }

	}
}

void BFS()
{
		int i;
		for(i = 0; i < nodeInfo.numNeighbours; i++)
		{
			if(0 == strcmp(nodeInfo.parentUID,nodeInfo.neighbourUIDs[i]))
			{
				//no need to send search message to node we marked as parent 
				continue;
			}

			struct Message msg = CreateMessage(SEARCH);
			msg.dstUID = atoi(nodeInfo.neighbourUIDs[i]);
			msg.round = nodeInfo.round;
			//Broadcast(msg);
			printf("<%s,%s,%d>\tSending Round %d %s Message to Neighbour with UID %d on Socket %d!\n", __FILE__, __func__, __LINE__, msg.round, msgType_str[msg.msgT],msg.dstUID,nodeInfo.neighbourSockets[i]);
			
			int error_code = send(nodeInfo.neighbourSockets[i], &msg, sizeof(msg),0);
			if(error_code < 0)
			{
				printf("<%s,%s,%d>\tFailed to Send Round %d %s Message to Neighbour With UID %s On Socket %s! Error_Code: %d\n", __FILE__, __func__, __LINE__,msg.round, msgType_str[msg.msgT],nodeInfo.neighbourUIDs[i], nodeInfo.neighbourSockets[i],error_code);
				exit(1);
			}
			else
			{
				printf("<%s,%s,%d>\tSuccessfully Sent Round %d %s Message to Neighbour With UID %d On Socket %d\n", __FILE__, __func__, __LINE__, msg.round, msgType_str[msg.msgT],msg.dstUID, nodeInfo.neighbourSockets[i]);
			}
		}
		nodeInfo.round = nodeInfo.round + 1;
}

void InitNode(char* machineName, char* pathToConfig)
{
	nodeInfo = Parse(machineName,pathToConfig);

	nodeInfo.mySocket = -1;
	int i;
	for (i = 0; i < nodeInfo.numNeighbours; i++){
		nodeInfo.neighbourSockets[i] = -1;
		nodeInfo.neighbourRepliedToSearch[i] = 0;
		nodeInfo.maxRoundsInNeighbours[i] = 0;
		strcpy(nodeInfo.childrenUIDs[i],"\0");
	}

	nodeInfo.numConnections = 0;
	nodeInfo.maxUIDSeen = atoi(nodeInfo.myUID);
	nodeInfo.maxDist = 0;
	nodeInfo.round = 1;
	nodeInfo.currDistToNode = 0;
	nodeInfo.currLeaderRoundCount = 0;
	nodeInfo.currMaxUIDRound = 0;
	nodeInfo.status = UNKNOWN;
}

void StartFlood()
{
	printf("<%s,%s,%d>\tNode With UID %s Starting FLOOD!\n", __FILE__, __func__, __LINE__,nodeInfo.myUID);

	while(UNKNOWN == nodeInfo.status)
	{
		struct Message msg = CreateMessage(FLOOD);
		msg.currMaxUID = nodeInfo.maxUIDSeen;
		msg.currDist = nodeInfo.currDistToNode;
		msg.currMaxDist = nodeInfo.maxDist;
		msg.round = nodeInfo.round;
		Broadcast(msg);
		
		sleep(2);

		//wait 
		while(0 == receivedFloodReplyFromAllNeighbours(msg.round))
		{
			printf("<%s,%s,%d> nodeInfo.maxRoundsInNeighbours[] = {", __FILE__, __func__, __LINE__);
			int i;
			for(i = 0; i < nodeInfo.numNeighbours; i++)
			{
				printf("%d\t",nodeInfo.maxRoundsInNeighbours[i]);
			}
			printf("}\n");
			sleep(2);
		}
	}
}

void StartFloodTerminate(){

	struct Message msg = CreateMessage(FLOOD_TERMINATE);
	msg.currMaxUID = nodeInfo.maxUIDSeen;
	msg.currDist = nodeInfo.currDistToNode;
	msg.currMaxDist = nodeInfo.maxDist;
	int currRound = getMaxRoundToStart();
	msg.round = currRound;
	Broadcast(msg);
}

void PrintHelp()
{
	printf("Expected:\n\t\tPelegsAlgorithm.o -i <Path/To/Config/File>\n\n");
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
		else
		{
			c = c + 1;
		}
	}

	printf("<%s,%s,%d>\tRetrieving Machine Hostname...\n",__FILE__,__func__,__LINE__);
	
	char hostname[100];
	gethostname(hostname,sizeof(hostname));

	char* machineName = strtok(hostname,".");
	if(NULL == machineName)
	{
		printf("<%s,%s,%d>\tError retrieving Machine Hostname...\n",__FILE__,__func__,__LINE__);
	}
	else
	{
		//do nothing
	}

	printf("<%s,%s,%d>\tInitializing Node For Machine %s\n",__FILE__,__func__,__LINE__,machineName);

	InitNode(machineName,pathToConfig);

	PrintNodeInfo(nodeInfo);

	nodeInfo.mySocket = CreateSocket(atoi(nodeInfo.myListeningPort));

	pthread_t connectToNodes_tid;

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
	 error_code = pthread_create(&connectToNodes_tid,&attr,(void *)ConnectToNeighbours,NULL);

	if(error_code != 0)
	{
		printf("<%s,%s,%d>\tFailed To Create ConnectToNodes Thread! Error_code: %d\n",__FILE__,__func__,__LINE__,error_code);
		exit(1);
	}
	else
	{
		printf("<%s,%s,%d>\tSuccessfully Created Thread for Connecting!\n",__FILE__,__func__,__LINE__);
		//do nothing
	}

	pthread_t acceptConnections_tid;
	
	//create thread for accepting incoming connections from clients
	error_code = pthread_create(&acceptConnections_tid,&attr,(void *)AcceptConnections,NULL);
	if(error_code != 0)
	{
		printf("<%s,%s,%d>\tFailed To Create AcceptConnection Thread! Error_code: %d\n",__FILE__,__func__,__LINE__,error_code);
		exit(1);
	}
	else
	{
		printf("<%s,%s,%d>\tSuccessfully Created Thread For Accepting Connections!\n",__FILE__,__func__,__LINE__);
		//do nothing
	}

	//pthread_join(connectToNodes_tid,NULL);
	//pthread_join(acceptConnections_tid,NULL);

	while(0 == allConnectionsEstablished())
	{
		printf("<%s,%s,%d>\tWaiting for All Connections to Be Established...\n",__FILE__,__func__,__LINE__);
		sleep(2);
	}


	StartFlood();

	while(nodeInfo.status == UNKNOWN);

	printf("<%s,%s,%d>\tStatus After Pelegs For Node %s = %s\n",__FILE__,__func__,__LINE__,nodeInfo.myUID,status_str[nodeInfo.status]);

	if(LEADER == nodeInfo.status)
	{
		sleep(3);
		BFS();
	}
	else
	{
		//do nothing - leader will initialize BFS
	}

	while(0 == receivedBFSReplyFromAllNeighbours())
	{
		printf("<%s,%s,%d>\tWaiting for All Neighbours To Reply To SEARCH...\n",__FILE__,__func__,__LINE__);
		sleep(2);
	}

	PrintNodeBFSInfo(nodeInfo);

	sleep(20);

	CloseConnections();

	return 0;
}

