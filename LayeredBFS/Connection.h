#ifndef CONNECTION_H
#define CONNECTION_H

int ValidatePort(int);
int ValidateIPAddress(char *);
int ResolveHostnameToIP(char* hostName, char* ipAddr);
int CreateSocket(int port);
int ConnectToNode(int uid ,char* hostName, int port);
int CloseConnection(int nodeSocket);

#endif