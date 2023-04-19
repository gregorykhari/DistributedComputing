#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

#include "Connection.h"

#define IP_ADDR INADDR_ANY

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

//converts the hostname of a machine to the ip address
int ResolveHostnameToIP(char * hostname , char* ip)
{
	struct hostent *he;
	struct in_addr **addr_list;
	int i;
		
	if ((he = gethostbyname( hostname )) == NULL) 
	{
		// get the host info
		printf("<%s,%s,%d>\tFailed to Get Host Address List For %s !\n",__FILE__,__func__,__LINE__,hostname);
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
		printf("<%s,%s,%d>\tFailed to Create Socket! Error: %d!\n",__FILE__,__func__,__LINE__,errno);
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

int ConnectToNode(int uid, char* hostName, int port)
{
	//printf("<%s,%s,%d>\tAttempting To Connect To Node With UID %d With Hostname %s On Port %d\n", __FILE__, __func__, __LINE__,uid,hostName,port);

	int error_code;
	int nodeSocket;
    char ipAddr[100];

    if(1 == ResolveHostnameToIP(hostName,ipAddr))
    {
        printf("<%s,%s,%d>\tFailed to Resolve Hostname %s For Node with UID %d to IPAddress!\n",__FILE__,__func__,__LINE__,hostName,uid);
        exit(1);
    }
    else
    {
        //do nothing 
    }

    if(0 == ValidateIPAddress(ipAddr))
    {
        printf("<%s,%s,%d>\tIP Adress %s is not in expected format of X.X.X.X !\n",__FILE__,__func__,__LINE__,ipAddr);
        printf("<%s,%s,%d>\tFailed to Connect to Node With UID %d!",__FILE__,__func__,__LINE__,uid);
        exit(1);
    }

    if(0 == ValidatePort(port))
    {
        printf("<%s,%s,%d>\tPort %d not within valid range of 1024 to 65535!\n",__FILE__,__func__,__LINE__,port);
        printf("<%s,%s,%d>\tFailed to Connect to Node With UID %d!\n",__FILE__,__func__,__LINE__,uid);
        exit(1);
    }

	printf("<%s,%s,%d>\tAttempting to Connect to Node With UID %d At IP Adress %s on Port %d!\n",__FILE__,__func__,__LINE__,uid,ipAddr,port);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET; //specified for IPv4 connection
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ipAddr);
		
	int connectionAttempts = 0;

    //create a network socket for TCP/IP communication 
    nodeSocket = socket(AF_INET,SOCK_STREAM,0);
    if (nodeSocket < 0)
    {
        printf("<%s,%s,%d>\tFailed to create socket! Error_code: %d .Error: %s!\n",__FILE__,__func__,__LINE__,error_code,strerror(errno));
    }
			
    error_code = connect(nodeSocket,(struct sockaddr *)&server_addr,sizeof(server_addr));
    if (error_code != 0)
    {
		printf("<%s,%s,%d>\tFailed to Connect to Node With UID %d. Error Code: %d !\n",__FILE__,__func__,__LINE__,uid,error_code);
        close(nodeSocket);
        return 0;
    }
    else
    {
        printf("<%s,%s,%d>\tSuccessfully Established Connection to Node At IPAddress %s on Port %d With Socket %d!\n",__FILE__,__func__,__LINE__,ipAddr,port,nodeSocket);
    }

    return nodeSocket;
}

int CloseConnection(int nodeSocket)
{
    return close(nodeSocket);
}