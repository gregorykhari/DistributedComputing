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


void printHelp();
int ValidatePort(int);
int ValidateIPAddress(char *);
int ResolveHostnameToIP(char* , char*);

int allConnectionsEstablished();
int CreateSocket(int port);
void ConnectToNeighbours();
void AcceptConnections();
void CloseConnections();
void CloseConnection(int idx);

void HandleMessages(void *);

void HandleSearchMessage(struct Message);
void HandleACKMessage(struct Message);
void HandleNACKMessage(struct Message);

struct Message createConnectionMessage();
struct Message createFloodMessage();
struct Message CreateFloodTerminationMessage();
struct Message createSearchMessage();
struct Message CreateACKMessage();
struct Message CreateNACKMessage();

void sendFloodMessage();
void sendFloodTerminationMessage();
bool isTerminationReached();

void PelegsAlgorithm(struct Message msg);
void BFS();

bool isSynchronized();

void startFlood();
void startFloodTerminate();
void sendToAllNeighbours(struct Message msg);

int allConnectionsEstablished()
{
	int flag = 1;
	int i;
	for(i = 0 ; i < nodeInfo.numNeighbours; i++)
	{
		if(nodeInfo.neighbourSockets[i] == -1)
		{
			flag = 0;
			break;
		}
	}
	return flag;
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

struct Message createConnectionMessage()
{
	struct Message msg;
	msg.srcUID = atoi(nodeInfo.myUID);
	msg.msgT = CONNECTION;
	return msg;
}

struct Message createFloodMessage(){
	struct Message msg;
	msg.srcUID = atoi(nodeInfo.myUID);
	msg.currMaxUID = nodeInfo.maxUIDSeen;
	msg.currDist = nodeInfo.currDistToNode;
	msg.currMaxDist = nodeInfo.maxDist;
	msg.msgT = FLOOD;
	return msg;
}

struct Message CreateFloodTerminationMessage(){
	struct Message msg;
	msg.srcUID = atoi(nodeInfo.myUID);
	msg.currMaxUID = nodeInfo.maxUIDSeen;
	msg.currDist = nodeInfo.currDistToNode;
	msg.currMaxDist = nodeInfo.maxDist;
	msg.msgT = FLOOD_TERMINATE;
	return msg;
}


struct Message CreateSearchMessage()
{
	struct Message msg;
	msg.srcUID = atoi(nodeInfo.myUID);
	msg.msgT = SEARCH;
	return msg;
}


struct Message CreateACKMessage()
{
	struct Message msg;
	msg.srcUID = atoi(nodeInfo.myUID);
	msg.msgT = ACK;
	return msg;
}

struct Message CreateNACKMessage()
{
	struct Message msg;
	msg.srcUID = atoi(nodeInfo.myUID);
	msg.msgT = ACK;
	return msg;
}

bool isTerminationReached(){
	//NEED to see the logic for this!
	return false;
}


int ResolveHostnameToIP(char * hostname , char* ip)
{
	struct hostent *he;
	struct in_addr **addr_list;
	int i;
		
	if ((he = gethostbyname( hostname )) == NULL) 
	{
		// get the host info
		printf("<%s,%s,%d> Failed to Get Host Address List!",__FILE__,__func__,__LINE__);
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
		printf("<%s,%s,%d>  Failed to create socket - error: %s!\n",__FILE__,__func__,__LINE__,strerror(errno));
		return -1;
	}
	else
	{
		printf("<%s,%s,%d>  Successfully created socket %d!\n",__FILE__,__func__,__LINE__,node_socket);
	}

	//assign server (IP) address to server socket
	int error_code = bind(node_socket,(struct sockaddr *) &server_addr,sizeof(server_addr));
	if (error_code != 0)
	{
		printf("<%s,%s,%d>  Failed to bind socket - error: %s, error_code: %d!\n",__FILE__,__func__,__LINE__,strerror(errno),error_code);
		return -1;
	}
	else
	{
		printf("<%s,%s,%d>  Successfully binded socket %d!\n",__FILE__,__func__,__LINE__,node_socket);
	}

    return node_socket;
}

//connects to randomly selected server with information returned by ConnectToServer
void sendToAllNeighbours(struct Message msg)
{
	int i;
	for( i = 0; i < nodeInfo.numNeighbours; i++){
		msg.dstUID = atoi(nodeInfo.neighbourUIDs[i]);
		printf("<%s,%s, %d>: Sending message type %d to Neighbour with UID %d on socket %d!\n", __FILE__, __func__, __LINE__, msg.msgT,msg.dstUID,nodeInfo.neighbourSockets[i]);
		
		int error_code = send(nodeInfo.neighbourSockets[i], &msg, sizeof(msg),0);
		if(error_code < 0)
		{
			printf("<%s,%s, %d>: Failed to send messages to neighbours: Error Code%d\n", __FILE__, __func__, __LINE__, error_code);
			exit(1);
		}
		else
		{
			//do nothing- message was sent successfully
			printf("<%s,%s, %d>: Successfully sent messages to neighbour %d: On socket:%d\n", __FILE__, __func__, __LINE__, msg.dstUID, nodeInfo.neighbourSockets[i]);

		}
	}
}
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
			printf("<%s,%s,%d> Failed to Resolve Hostname %s to IPAddress!",__FILE__,__func__,__LINE__,hostname);
		}
		else
		{
			//do nothing 
		}

		int port = atoi(nodeInfo.neighbourListeningPorts[node_socket_index]);

		if(0 == ValidateIPAddress(ipAddr))
		{
			printf("<%s,%s,%d> IP Adress %s is not in expected format of X.X.X.X !\n",__FILE__,__func__,__LINE__,ipAddr);
			printf("<%s,%s,%d> Failed to Connect to Node!",__FILE__,__func__,__LINE__);
			exit(1);
		}

		if(0 == ValidatePort(port))
		{
			printf("<%s,%s,%d> Port %d not within valid range of 1024 to 65535!\n",__FILE__,__func__,__LINE__,port);
			printf("<%s,%s,%d> Failed to Connect to Node!",__FILE__,__func__,__LINE__);
			exit(1);
		}

		printf("<%s,%s,%d> Attempting to Connect to IP Adress %s on Port %d!\n",__FILE__,__func__,__LINE__,ipAddr,port);

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
				printf("<%s,%s,%d> Failed to create socket - error: %s!\n",__FILE__,__func__,__LINE__,strerror(errno));
			}
			

			error_code = connect(nodeInfo.neighbourSockets[node_socket_index],(struct sockaddr *)&server_addr,sizeof(server_addr));
			if (error_code != 0)
			{
				close(nodeInfo.neighbourSockets[node_socket_index]);
			}
			else
			{
				printf("<%s,%s,%d> Successfully established connection to %s on port %d !\n",__FILE__,__func__,__LINE__,ipAddr,port);
				printf("<%s,%s,%d> Successfully Established Connection at Socket = %d\n",__FILE__,__func__,__LINE__,nodeInfo.neighbourSockets[node_socket_index]);
				break;
			}
		}
			
		//send message to other node to let it know which socket to assign to
		struct Message msg = createConnectionMessage();
		printf("My current UID : %d\n",msg.srcUID);
		error_code = send(nodeInfo.neighbourSockets[node_socket_index], &msg, sizeof(msg),0);
		if (error_code < 0)
		{
			printf("<%s,%s,%d> Error Sending Message (%s) to Server on Socket %d - error: %s!\n",__FILE__,__func__,__LINE__,send_buffer,nodeInfo.neighbourSockets[node_socket_index],strerror(errno));
		}
		else
		{
			//do nothing
		}

		//create a variable to pass network socket to thread args
		int* nsi = malloc(sizeof(int));
		if(NULL == nsi)
		{
			printf("<%s,%s,%d> Failed to malloc for nsi!\n",__FILE__,__func__,__LINE__);
		}
		else
		{
				//do nothing
		}

		*nsi = node_socket_index;

		pthread_attr_t attr;                                                                                                                

		error_code = pthread_attr_init(&attr);                                               
		if (error_code == -1) {                                                              
			printf("<%s,%s,%d> Failed to create HandleMessages thread for Client at socket %d - error: %d\n",__FILE__,__func__,__LINE__,error_code,nodeInfo.neighbourSockets[node_socket_index]);
			exit(1);                                                                  
		}                                                                            

		int ds = PTHREAD_CREATE_DETACHED;                                                                      
		error_code = pthread_attr_setdetachstate(&attr, ds);                                
		if (error_code == -1) {                                                              
			printf("<%s,%s,%d> Failed to create HandleMessages thread for Client at socket %d - error: %d\n",__FILE__,__func__,__LINE__,error_code,nodeInfo.neighbourSockets[node_socket_index]);
			exit(1);                                                                  
		}     

		//create thread for receiving data from servers
		error_code = pthread_create(&nodeInfo.neighbourThreads[node_socket_index],&attr,(void *)HandleMessages,nsi);
		if(error_code != 0)
		{
			printf("<%s,%s,%d> Failed to create HandleServer thread for Client at socket %d - error: %d\n",__FILE__,__func__,__LINE__,error_code,nodeInfo.neighbourSockets[node_socket_index]);
			exit(1);
		}
		else
		{
			//do nothing
		}
	}
}


void AcceptConnections()
{

	//listen for incoming connections on socket
	int error_code = listen(nodeInfo.mySocket,1);
	if (error_code != 0)
	{
		printf("<%s,%s,%d> Failed to initiate listening on socket - error: %s!\n",__FILE__,__func__,__LINE__,strerror(errno));
		exit(1);
	}
	else
	{
		printf("<%s,%s,%d> Successfully initiated listening on socket %d!\n",__FILE__,__func__,__LINE__,nodeInfo.mySocket);
	}

	while(1)
	{
		char recv_buffer[BUFFER_SIZE];

		printf("<%s,%s,%d> Checkpoint\n",__FILE__,__func__,__LINE__);

		//accept incoming connection request from client
		int node_socket = accept(nodeInfo.mySocket,NULL,NULL);
		if (node_socket < 0)
		{
			printf("<%s,%s,%d> Failed to accept incoming connection from client - error: %s!\n",__FILE__,__func__,__LINE__,strerror(errno));
			exit(1);
		}
		else
		{
			//do nothing
			printf("<%s,%s,%d> Successfully established connection with client at socket %d!\n",__FILE__,__func__,__LINE__,node_socket);
		}

		printf("<%s,%s,%d> Checkpoint\n",__FILE__,__func__,__LINE__);

		memset(recv_buffer,'\0',sizeof(recv_buffer));
		struct Message recv_msg;
		error_code = recv(node_socket,&recv_msg,sizeof(recv_buffer),0);

		if(error_code < 0)
		{
			printf("<%s,%s,%d> Error while receiving message from Server at socket %d. Error %d : %s!\n",__FILE__,__func__,__LINE__,node_socket,error_code,strerror(errno));
			return;
		}
		else
		{
			//printf("<%s,%s,%d> Received Message (%s) from Server at Socket %d!\n",__FILE__,__func__,__LINE__,recv_buffer,node_socket);
			printf("Received message size :%lu\n", sizeof(recv_msg));
			printf("Message Types: %d\n", recv_msg.msgT);
		}

		printf("<%s,%s,%d> Checkpoint\n",__FILE__,__func__,__LINE__);

		char node_uid[BUFFER_SIZE];
		sprintf(node_uid, "%d", recv_msg.srcUID);

		printf("<%s,%s,%d> node_uid= %s\n",__FILE__,__func__,__LINE__,node_uid);

		int neighbourIndex = 0;
		int i;
		for(i = 0; i < nodeInfo.numNeighbours; i++)
		{
			if(0 == strcmp(nodeInfo.neighbourUIDs[i],node_uid))
			{
				neighbourIndex = i;
				nodeInfo.neighbourSockets[neighbourIndex] = node_socket;
				break;
			}
		}


		printf("<%s,%s,%d> Successfully Established Connection at Socket = %d\n",__FILE__,__func__,__LINE__,nodeInfo.neighbourSockets[neighbourIndex]);

		printf("<%s,%s,%d> Checkpoint\n",__FILE__,__func__,__LINE__);

		//create a variable to pass network socket to thread args
		int* ni = malloc(sizeof(int));
		if(NULL == ni)
		{
			printf("<%s,%s,%d> Failed to malloc for nsi for socket %d!\n",__FILE__,__func__,__LINE__,node_socket);
		}
		else
		{
				//do nothing
		}

		*ni = neighbourIndex;

		printf("<%s,%s,%d> Neighbour Indexes :%d\t :%d\n",__FILE__,__func__,__LINE__, neighbourIndex,*ni);
		pthread_attr_t attr;                                                                                                                

		printf("<%s,%s,%d> Checkpoint\n",__FILE__,__func__,__LINE__);

		error_code = pthread_attr_init(&attr);                                               
		if (error_code == -1) {                                                              
			printf("<%s,%s,%d> Failed to create HandleMessages thread for Client at socket %d! Error_code: %d, Error: %s \n",__FILE__,__func__,__LINE__,error_code,nodeInfo.neighbourSockets[neighbourIndex],strerror(errno));
			exit(1);                                                                  
		}                                                                            

		int ds = PTHREAD_CREATE_DETACHED;                                                                      
		error_code = pthread_attr_setdetachstate(&attr, ds);                                
		if (error_code == -1) {                                                              
			printf("<%s,%s,%d> Failed to create HandleMessages thread for Client at socket %d! Error_code: %d, Error: %s \n",__FILE__,__func__,__LINE__,error_code,nodeInfo.neighbourSockets[neighbourIndex],strerror(errno));
			exit(1);                                                                  
		}     

		printf("<%s,%s,%d> Checkpoint\n",__FILE__,__func__,__LINE__);


		//create thread for receiving data from servers
		error_code = pthread_create(&nodeInfo.neighbourThreads[neighbourIndex],&attr,(void *)HandleMessages,ni);
		if(error_code != 0)
		{
			printf("<%s,%s,%d> Failed to create HandleServer thread for Client at socket %d! Error_code: %d, Error: %s \n",__FILE__,__func__,__LINE__,error_code,nodeInfo.neighbourSockets[neighbourIndex],strerror(errno));
			exit(1);
		}
		else
		{
			//do nothing
		}

		printf("<%s,%s,%d> Checkpoint\n",__FILE__,__func__,__LINE__);

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
		printf("<%s,%s,%d> Failed to receive message!\t %d\n",__FILE__,__func__,__LINE__,error_code);
		return;                                                                
	}
	else
	{
		printf("<%s,%s,%d> Successfully Received %s Message from Neighbour with UID %d on Socket %d\n",__FILE__,__func__,__LINE__,msgType_str[recv_msg.msgT],nodeInfo.neighbourSockets[neighbourIndex]);
	}

	while(1)
	{
		while(0 == isSynchronized());
		
		switch (recv_msg.msgT)
		{
			case CONNECTION:
				printf("Synchronized:- %d\n", isSynchronized());
				break;
			
			case FLOOD:
				PelegsAlgorithm(recv_msg);
				break;

			case FLOOD_TERMINATE:
				if(nodeInfo.status == UNKNOWN){
					nodeInfo.status = NON_LEADER;
					struct Message reply_msg = CreateFloodTerminationMessage();
					printf("<%s,%s, %d>: FLOOD TERMINATE: Sending to all neighbours\n", __FILE__, __func__, __LINE__);	
					sendToAllNeighbours(reply_msg);
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
		}

		memset(recv_buffer,'\0',sizeof(recv_buffer));
		int error_code = recv(nodeInfo.neighbourSockets[neighbourIndex], &recv_buffer,sizeof(recv_buffer),0);
		recv_msg = *((struct Message *)recv_buffer);
		if (error_code < 0) 
		{                                                              
			printf("<%s,%s,%d> Failed To Receive Message From Neighbour with UID %s On Socket %d!\n",__FILE__,__func__,__LINE__,nodeInfo.neighbourUIDs[neighbourIndex],nodeInfo.neighbourSockets[neighbourIndex]);
			exit(1);                                                                  
		}
		else
		{
			printf("<%s,%s,%d> Successfully Received %s Message from Neighbour with UID %d on Socket %d\n",__FILE__,__func__,__LINE__,msgType_str[recv_msg.msgT],nodeInfo.neighbourSockets[neighbourIndex]);
		}
	}
}

void HandleSearchMessage(struct Message msg)
{

	char srcUID[BUFFER_SIZE];
	sprintf(srcUID,"%d",msg.srcUID);
	printf("<%s,%s,%d> Received SEARCH Message from UID %s!",__FILE__,__func__,__LINE__,srcUID);

	if(1 == nodeInfo.marked)
	{
		printf("<%s,%s,%d> Already Marked For Parent UID %s!",__FILE__,__func__,__LINE__,nodeInfo.parentUID);

		//determine socket to send NACK message back to base on recv_msg srcUID
		int i,socketIndex;

		for(i = 0; i < nodeInfo.numNeighbours; i++)
		{
			if(0 == strcmp(nodeInfo.neighbourUIDs[i],srcUID))
			{
				socketIndex = i;
				break;
			}
			else
			{
				//do nothing
			}
		}

		//check if we already sent search messages to all neighbours
		struct Message msg = CreateNACKMessage();
		int error_code = send(nodeInfo.neighbourSockets[socketIndex],&msg,sizeof(msg),0);
		if(error_code < 0)
		{
			printf("<%s,%s,%d> Failed to Send NACK Message to Neighbour with UID %s on Socket %d!",__FILE__,__func__,__LINE__,nodeInfo.neighbourUIDs[socketIndex],nodeInfo.neighbourSockets[socketIndex]);
			exit(1);
		}
		else
		{
			//do nothing- message was sent successfully
			printf("<%s,%s,%d> Successfully Sent NACK Message to Neighbour with UID %s on Socket %d!",__FILE__,__func__,__LINE__,nodeInfo.neighbourUIDs[socketIndex],nodeInfo.neighbourSockets[socketIndex]);
		}
	}
	else
	{
		//mark the first node we received a search message from as parent
		nodeInfo.marked = 1;
		char parentUID[10];
		sprintf(parentUID,"%d",msg.srcUID);
		strcpy(nodeInfo.parentUID,parentUID);

		//send search message to all children
		BFS();
	}
}

void HandleACKMessage(struct Message msg)
{
	char srcUID[10];
	sprintf(srcUID,"%d",msg.srcUID);

	int i;
	//set that neighbour has replied
	for(i = 0; i < nodeInfo.numNeighbours; i++)
	{
		if(0 == strcmp(nodeInfo.neighbourUIDs[i],srcUID))
		{
			nodeInfo.neighbourRepliedToSearch[i] = 1;
			break;
		}
	}

	//put childUID in first available slot marked for children
	for(i = 0; i < nodeInfo.numNeighbours; i++)
	{
		if(0 == strcmp(nodeInfo.childrenUIDs[i],"\0"))
		{
			strcpy(nodeInfo.childrenUIDs[i],srcUID);
			break;
		}
	}

	//check if all neighbours have now replied
	int flag = 1;
	for(i = 0; i < nodeInfo.numNeighbours; i++)
	{
		if(0 == nodeInfo.neighbourRepliedToSearch[i])
		{
			flag = 0;
		}
	}

	//if all neighbours have now replied, send ACK up to parent
	if(1 == flag)
	{
		//perform converge cast and finally send ACK to parent
		CreateACKMessage();
	}
	else
	{
		//do nothing - still need more replies to proceed
	}
	
}

void HandleNACKMessage(struct Message msg)
{
	int i;

	char srcUID[10];
	sprintf(srcUID,"%d",msg.srcUID);

	//set that neighbour has replied
	for(i = 0; i < nodeInfo.numNeighbours; i++)
	{
		if(0 == strcmp(nodeInfo.neighbourUIDs[i],srcUID))
		{
			nodeInfo.neighbourRepliedToSearch[i] = 1;
			break;
		}
	}
	

	//check if all neighbours have now replied
	int flag = 1;
	for(i = 0; i < nodeInfo.numNeighbours; i++)
	{
		if(0 == nodeInfo.neighbourRepliedToSearch[i])
		{
			flag = 0;
		}
	}

	if(1 == flag)
	{
		//perform converge cast and finally send ACK to parent
		CreateACKMessage();
	}
	else
	{
		//do nothing - still need more replies to proceed
	}
}

bool isSynchronized(){
	int currMin = nodeInfo.maxRoundsInNeighbours[0];
	int currMax = nodeInfo.maxRoundsInNeighbours[0];

	int i;
	for (i = 0 ;i < nodeInfo.numNeighbours; i++){

		if (nodeInfo.maxRoundsInNeighbours[i] == -1){
			return false;
		}
		if (nodeInfo.maxRoundsInNeighbours[i] < currMin){
			currMin = nodeInfo.maxRoundsInNeighbours[i];
		}
		
		if(nodeInfo.maxRoundsInNeighbours[i] > currMax){
			currMax = nodeInfo.maxRoundsInNeighbours[i];
		}
	}

	if ((currMax - currMin) >= 2){
		return false;
	}
	else{
		return true;
	}
}

void CloseConnections()
{
	int i = 0;
	struct Message closeConnectionMsg = createConnectionMessage();
	closeConnectionMsg.msgT = CLOSE;
	sendToAllNeighbours(closeConnectionMsg);
	for(i = 0; i < nodeInfo.numNeighbours; i++)
	{
		close(nodeInfo.neighbourSockets[i]);
		printf("<%s,%s,%d> Successfully Closed Connections to Neighbour with UID %s",__FILE__,__func__,__LINE__,nodeInfo.neighbourUIDs[i]);
	}
}

void CloseConnection(int index)
{
	close(nodeInfo.neighbourSockets[index]);
	printf("<%s,%s,%d> Successfully Closed Connections to Neighbour with UID %s",__FILE__,__func__,__LINE__,nodeInfo.neighbourUIDs[index]);
}

void PelegsAlgorithm(struct Message msg)
{
	if(nodeInfo.maxUIDSeen == msg.currMaxUID){
		nodeInfo.currLeaderCount++;
	}
	
	if(nodeInfo.currLeaderCount > 3 && nodeInfo.maxUIDSeen == msg.currMaxUID){
		//Start a flood terminate message!
		printf("<%s,%s, %d>: Start the flood Terminate!\n", __FILE__, __func__, __LINE__);
		nodeInfo.status = LEADER;
		startFloodTerminate();
	}
	else{
		if(nodeInfo.maxUIDSeen < msg.currMaxUID){
			printf("<%s,%s, %d>: Received Message with larger UID\t Need to update!\n", __FILE__, __func__, __LINE__);
			nodeInfo.maxUIDSeen = msg.currMaxUID;
			nodeInfo.currDistToNode = msg.currDist + 1;
			nodeInfo.maxDist = nodeInfo.currDistToNode;
			nodeInfo.currLeaderCount = 1;
		}
		else if(nodeInfo.maxUIDSeen == msg.currMaxUID){
			printf("<%s,%s, %d>: Received Message with the same UID\t Setting the max distance\n", __FILE__, __func__, __LINE__);
			if(msg.currMaxDist > nodeInfo.maxDist)
				nodeInfo.maxDist = msg.currMaxDist;
			printf("<%s,%s, %d>: RECV Same UID\t maxDistance: %d\n", __FILE__, __func__, __LINE__, nodeInfo.maxDist);
		}
		struct Message reply_msg = createFloodMessage();
		sendToAllNeighbours(reply_msg);
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

			struct Message msg = CreateSearchMessage();
			msg.dstUID = atoi(nodeInfo.neighbourUIDs[i]);

			int error_code = send(nodeInfo.neighbourSockets[i],&msg,sizeof(msg),0);
			if(error_code < 0)
			{
				printf("<%s,%s,%d> Failed to Send SEARCH Message to Neighbour with UID %s on Socket %d!",__FILE__,__func__,__LINE__,nodeInfo.neighbourUIDs[i],nodeInfo.neighbourSockets[i]);
				exit(1);
			}
			else
			{
				//do nothing- message was sent successfully
				printf("<%s,%s,%d> Successfully Sent SEARCH Message to Neighbour with UID %s on Socket %d!",__FILE__,__func__,__LINE__,nodeInfo.neighbourUIDs[i],nodeInfo.neighbourSockets[i]);
			}
		}
}

void initNode(){
	nodeInfo.mySocket = -1;
	int i;
	for (i = 0; i < nodeInfo.numNeighbours; i++){
		nodeInfo.neighbourSockets[i] = -1;
		nodeInfo.neighbourRepliedToSearch[i] = 0;
		nodeInfo.maxRoundsInNeighbours[i] = 0;
		strcpy(nodeInfo.childrenUIDs[i],"\0");
	}

	nodeInfo.maxUIDSeen = atoi(nodeInfo.myUID);
	nodeInfo.maxDist = 0;
	nodeInfo.currDistToNode = 0;
	nodeInfo.currLeaderCount= 1;
	nodeInfo.status = UNKNOWN;
}

void startFlood()
{

	printf("<%s,%s, %d>: Starting FLOOD!\n", __FILE__, __func__, __LINE__);

	struct Message msg = createFloodMessage();

	sendToAllNeighbours(msg);
	printf("<%s,%s, %d>: For UID:%s \tSent message to all neighbours!\n", __FILE__, __func__, __LINE__, nodeInfo.myUID);
}

void startFloodTerminate(){

	struct Message msg = CreateFloodTerminationMessage();
	sendToAllNeighbours(msg);
	printf("<%s,%s, %d>: For UID:%s \tsend message to all neighbours!\n", __FILE__, __func__, __LINE__, nodeInfo.myUID);
}

void printHelp()
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
			printHelp();
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

	printf("<%s,%s,%d> Retrieving Machine Hostname...\n",__FILE__,__func__,__LINE__);
	
	char hostname[100];
	gethostname(hostname,sizeof(hostname));

	char* machineName = strtok(hostname,".");
	if(NULL == machineName)
	{
		printf("<%s,%s,%d> Error retrieving Machine Hostname...\n",__FILE__,__func__,__LINE__);
	}
	else
	{
		//do nothing
	}

	printf("<%s,%s,%d> Initializing Node For Machine %s\n",__FILE__,__func__,__LINE__,machineName);

	nodeInfo = Parse(machineName,pathToConfig);

	initNode();

	PrintNodeInfo(nodeInfo);

	nodeInfo.mySocket = CreateSocket(atoi(nodeInfo.myListeningPort));

	pthread_t connectToNodes_tid;

	pthread_attr_t attr;                                                                                                                

	int error_code = pthread_attr_init(&attr);                                               
		if (error_code == -1) {                                                              
			printf("<%s,%s,%d> Failed to create HandleMessages thread for Client at soc - error: %d\n",__FILE__,__func__,__LINE__,error_code);
			exit(1);                                                                  
		}                                                                            

		int ds = PTHREAD_CREATE_DETACHED;                                                                      
		error_code = pthread_attr_setdetachstate(&attr, ds);                                
		if (error_code == -1) {                                                              
			printf("<%s,%s,%d> Failed to create HandleMessages thread for Client at soc- error: %d\n",__FILE__,__func__,__LINE__,error_code);
			exit(1);                                                                  
		}    

	//create thread for accepting incoming connections from clients
	 error_code = pthread_create(&connectToNodes_tid,&attr,(void *)ConnectToNeighbours,NULL);

	if(error_code != 0)
	{
		printf("<%s,%s,%d> Failed To Create ConnectToNodes Thread - error: %d\n",__FILE__,__func__,__LINE__,error_code);
		exit(1);
	}
	else
	{
		printf("<%s,%s,%d> Successfully created thread for connecting!\n",__FILE__,__func__,__LINE__);
		//do nothing
	}

	pthread_t acceptConnections_tid;
	
	//create thread for accepting incoming connections from clients
	error_code = pthread_create(&acceptConnections_tid,&attr,(void *)AcceptConnections,NULL);
	if(error_code != 0)
	{
		printf("<%s,%s,%d> Failed To Create AcceptConnection Thread - error: %d\n",__FILE__,__func__,__LINE__,error_code);
		exit(1);
	}
	else
	{
		printf("<%s,%s,%d> Successfully created thread for accepting connections!\n",__FILE__,__func__,__LINE__);
		//do nothing
	}

	pthread_join(connectToNodes_tid,NULL);
	pthread_join(acceptConnections_tid,NULL);


	printf("<%s,%s,%d> Waiting for All Connections to Be Established...\n",__FILE__,__func__,__LINE__);
	sleep(5);

	
	//Pelegs();
	startFlood();

	while(UNKNOWN == nodeInfo.status);

	if(LEADER == nodeInfo.status)
	{
		sleep(10);
		BFS();

	}
	else
	{
		//do nothing - leader will initialize BFS
	}

	PrintNodeBFSInfo(nodeInfo);

	sleep(10);

	CloseConnections();

	return 0;
}

