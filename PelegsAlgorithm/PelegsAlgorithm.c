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

#include <errno.h>
#include <string.h>

#include <unistd.h>
#include <pthread.h>
#include "Node.h"

#include "ConfigParser.h"

#define IP_ADDR INADDR_ANY
#define BUFFER_SIZE 200

static struct Node nodeInfo;

void printHelp();
int ValidatePort(int);
int ValidateIPAddress(char *);
int ResolveHostnameToIP(char* , char*);
int CreateSocket(int port);
void ConnectToNeighbours();
void AcceptConnections();
void HandleMessages(void *);

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

int ResolveHostnameToIP(char * hostname , char* ip)
{
	struct hostent *he;
	struct in_addr **addr_list;
	int i;
		
	if ((he = gethostbyname( hostname )) == NULL) 
	{
		// get the host info
		printf("<%s,%d> Failed to Get Host Address List!",__FILE__,__LINE__);
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
		printf("<%s,%d>  Failed to create socket - error: %s!\n",__FILE__,__LINE__,strerror(errno));
		return -1;
	}
	else
	{
		printf("<%s,%d>  Successfully created socket %d!\n",__FILE__,__LINE__,node_socket);
	}

	//assign server (IP) address to server socket
	int error_code = bind(node_socket,(struct sockaddr *) &server_addr,sizeof(server_addr));
	if (error_code != 0)
	{
		printf("<%s,%d>  Failed to bind socket - error: %s, error_code: %d!\n",__FILE__,__LINE__,strerror(errno),error_code);
		return -1;
	}
	else
	{
		printf("<%s,%d>  Successfully binded socket %d!\n",__FILE__,__LINE__,node_socket);
	}

    return node_socket;
}

//connects to randomly selected server with information returned by ConnectToServer
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
			printf("<%s,%d> Failed to Resolve Hostname %s to IPAddress!",__FILE__,__LINE__,hostname);
		}
		else
		{
			//do nothing 
		}

		int port = atoi(nodeInfo.neighbourListeningPorts[node_socket_index]);

		if(0 == ValidateIPAddress(ipAddr))
		{
			printf("<%s,%d> IP Adress %s is not in expected format of X.X.X.X !\n",__FILE__,__LINE__,ipAddr);
			printf("<%s,%d> Failed to Connect to Node!",__FILE__,__LINE__);
			exit(1);
		}

		if(0 == ValidatePort(port))
		{
			printf("<%s,%d> Port %d not within valid range of 1024 to 65535!\n",__FILE__,__LINE__,port);
			printf("<%s,%d> Failed to Connect to Node!",__FILE__,__LINE__);
			exit(1);
		}

		printf("<%s,%d> Attempting to Connect to IP Adress %s on Port %d!\n",__FILE__,__LINE__,ipAddr,port);

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
				printf("<%s,%d> Failed to create socket - error: %s!\n",__FILE__,__LINE__,strerror(errno));
			}
			else
			{
				printf("<%s,%d> Successfully created socket!\n",__FILE__,__LINE__);
			}

			error_code = connect(nodeInfo.neighbourSockets[node_socket_index],(struct sockaddr *)&server_addr,sizeof(server_addr));
			if (error_code != 0)
			{

			}
			else
			{
				printf("<%s,%d> Successfully established connection to %s on port %d !\n",__FILE__,__LINE__,ipAddr,port);
				break;
			}
		}
			
		//send message to other node to let it know which socket to assign to
		memset(send_buffer,'\0',sizeof(send_buffer));
		sprintf(send_buffer,"INCOMING_CONNECTION:%s",nodeInfo.myUID);

		printf("<%s,%d> Sending Message (%s) to Server on Socket %d!\n",__FILE__,__LINE__,send_buffer,nodeInfo.neighbourSockets[node_socket_index]);

		error_code = send(nodeInfo.neighbourSockets[node_socket_index],send_buffer,sizeof(send_buffer),0);
		if (error_code < 0)
		{
			printf("<%s,%d> Error Sending Message (%s) to Server on Socket %d - error: %s!\n",__FILE__,__LINE__,send_buffer,nodeInfo.neighbourSockets[node_socket_index],strerror(errno));
		}
		else
		{
			//do nothing
		}

		//create a variable to pass network socket to thread args
		int* nsi = malloc(sizeof(int));
		if(NULL == nsi)
		{
			printf("<%s,%d> Failed to malloc for nsi!\n",__FILE__,__LINE__);
		}
		else
		{
				//do nothing
		}

		*nsi = node_socket_index;

		pthread_attr_t attr;                                                                                                                

		error_code = pthread_attr_init(&attr);                                               
		if (error_code == -1) {                                                              
			printf("<%s,%d> Failed to create HandleMessages thread for Client at socket %d - error: %d\n",__FILE__,__LINE__,error_code,nodeInfo.neighbourSockets[node_socket_index]);
			exit(1);                                                                  
		}                                                                            

		int ds = PTHREAD_CREATE_DETACHED;                                                                      
		error_code = pthread_attr_setdetachstate(&attr, ds);                                
		if (error_code == -1) {                                                              
			printf("<%s,%d> Failed to create HandleMessages thread for Client at socket %d - error: %d\n",__FILE__,__LINE__,error_code,nodeInfo.neighbourSockets[node_socket_index]);
			exit(1);                                                                  
		}     

		//create thread for receiving data from servers
		error_code = pthread_create(&nodeInfo.neighbourThreads[node_socket_index],&attr,(void *)HandleMessages,nsi);
		if(error_code != 0)
		{
			printf("<%s,%d> Failed to create HandleServer thread for Client at socket %d - error: %d\n",__FILE__,__LINE__,error_code,nodeInfo.neighbourSockets[node_socket_index]);
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
	while(1)
	{

		char recv_buffer[BUFFER_SIZE];

		//listen for incoming connections on socket
		int error_code = listen(nodeInfo.mySocket,1);
		if (error_code != 0)
		{
			printf("<%s,%d> Failed to initiate listening on socket - error: %s!\n",__FILE__,__LINE__,strerror(errno));
			exit(1);
		}
		else
		{
			printf("<%s,%d> Successfully initiated listening on socket %d!\n",__FILE__,__LINE__,nodeInfo.mySocket);
		}

		//accept incoming connection request from client
		int node_socket = accept(nodeInfo.mySocket,NULL,NULL);
		if (node_socket < 0)
		{
			printf("<%s,%d> Failed to accept incoming connection from client - error: %s!\n",__FILE__,__LINE__,strerror(errno));
			exit(1);
		}
		else
		{
			//do nothing
			printf("<%s,%d> Successfully established connection with client at socket %d!\n",__FILE__,__LINE__,node_socket);
		}

		memset(recv_buffer,'\0',sizeof(recv_buffer));
		error_code = recv(node_socket,&recv_buffer,sizeof(recv_buffer),0);

		if(error_code < 0)
		{
			printf("<%s,%d> Error while receiving message from Server at socket %d. Error %d : %s!\n",__FILE__,__LINE__,node_socket,error_code,strerror(errno));
			return;
		}
		else
		{
			printf("<%s,%d> Received Message (%s) from Server at Socket %d!\n",__FILE__,__LINE__,recv_buffer,node_socket);
		}

		char* node_uid;
		if(NULL != strstr(recv_buffer,"INCOMING_CONNECTION"))
		{
			strtok(recv_buffer,":");
			
			if(NULL == (node_uid = strtok(NULL,":")))
			{
				printf("<%s,%d> Failed to get UID!",__FILE__,__LINE__);
			}
			else
			{

			}
		}

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

		//create a variable to pass network socket to thread args
		int* ni = malloc(sizeof(int));
		if(NULL == ni)
		{
			printf("<%s,%d> Failed to malloc for nsi for socket %d!\n",__FILE__,__LINE__,node_socket);
		}
		else
		{
				//do nothing
		}

		*ni = neighbourIndex;

		pthread_attr_t attr;                                                                                                                

		error_code = pthread_attr_init(&attr);                                               
		if (error_code == -1) {                                                              
			printf("<%s,%d> Failed to create HandleMessages thread for Client at socket %d - error: %d\n",__FILE__,__LINE__,error_code,nodeInfo.neighbourSockets[neighbourIndex]);
			exit(1);                                                                  
		}                                                                            

		int ds = PTHREAD_CREATE_DETACHED;                                                                      
		error_code = pthread_attr_setdetachstate(&attr, ds);                                
		if (error_code == -1) {                                                              
			printf("<%s,%d> Failed to create HandleMessages thread for Client at socket %d - error: %d\n",__FILE__,__LINE__,error_code,nodeInfo.neighbourSockets[neighbourIndex]);
			exit(1);                                                                  
		}     

		//create thread for receiving data from servers
		error_code = pthread_create(&nodeInfo.neighbourThreads[neighbourIndex],&attr,(void *)HandleMessages,ni);
		if(error_code != 0)
		{
			printf("<%s,%d> Failed to create HandleServer thread for Client at socket %d - error: %d\n",__FILE__,__LINE__,error_code,nodeInfo.neighbourSockets[neighbourIndex]);
			exit(1);
		}
		else
		{
			//do nothing
		}
	}
}

void HandleMessages(void *ni)
{
	int neighbourIndex = *((int*)ni);

	char send_buffer[BUFFER_SIZE];
	char recv_buffer[BUFFER_SIZE];

	printf("<%s,%d> in HandleMessage\n",__FILE__,__LINE__);
}

void PelegsAlgorithm()
{
	//wait for all nodes to be connected
	while(1)
	{
		int i;
		int done = 1;
		for(i = 0; i < nodeInfo.numNeighbours; i++)
		{
			if(-1 == nodeInfo.neighbourSockets[i])
			{
				done = 0;
			}
		}
		if(1 == done)
		{
			break;
		}
	}


}
void printHelp()
{
	printf("Expected:\n\t\tPelegsAlgorithm.o -u <NodeUID> -i <Path/To/Config/File>\n\n");
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
		else if(0 == strcmp(argv[c],"-u"))
		{
			c = c + 1;
			myUID = argv[c];
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

	printf("<%s,%d> Initializing Node With UID %s\n",__FILE__,__LINE__,myUID);

	nodeInfo = Parse(myUID,pathToConfig);

	PrintNodeInfo(nodeInfo);

	nodeInfo.mySocket = CreateSocket(atoi(nodeInfo.myListeningPort));

	pthread_t connectToNodes_tid;

	//create thread for accepting incoming connections from clients
	int error_code = pthread_create(&connectToNodes_tid,NULL,(void *)ConnectToNeighbours,NULL);

	if(error_code != 0)
	{
		printf("<%s,%d> Failed To Create ConnectToNodes Thread - error: %d\n",__FILE__,__LINE__,error_code);
		exit(1);
	}
	else
	{
		printf("<%s,%d> Successfully created thread for connecting!\n",__FILE__,__LINE__);
		//do nothing
	}

	pthread_t acceptConnections_tid;
	
	//create thread for accepting incoming connections from clients
	error_code = pthread_create(&acceptConnections_tid,NULL,(void *)AcceptConnections,NULL);
	if(error_code != 0)
	{
		printf("<%s,%d> Failed To Create AcceptConnection Thread - error: %d\n",__FILE__,__LINE__,error_code);
		exit(1);
	}
	else
	{
		printf("<%s,%d> Successfully created thread for accepting connections!\n",__FILE__,__LINE__);
		//do nothing
	}

	pthread_join(connectToNodes_tid,NULL);
	pthread_join(acceptConnections_tid,NULL);

	return 0;
}

