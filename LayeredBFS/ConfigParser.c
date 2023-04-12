#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ConfigParser.h"

int isValidLine(char* line);

char* removeLeadingTrailingWhitespace(char* line);

struct _Node Parse(char* pathToConfig, int myUID)
{
    struct _Node node;

    FILE *fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    int nodeUIDs[MAX_NEIGHBOURS];
    char nodeHostNames[MAX_NEIGHBOURS][CHAR_BUFFER_SIZE];
	int nodeListeningPorts[MAX_NEIGHBOURS];

    int numNodes;
    int k,hostK;
    int LaFlag = 0, LbFlag = 0;

    printf("<%s,%s,%d>\tOpening Config File %s!\n",__FILE__,__func__,__LINE__,pathToConfig);

    if(NULL == (fp = fopen(pathToConfig,"r")))
    {
        printf("<%s,%s,%d>\tERROR: Failed to open config file %s for reading!",__FILE__,__func__,__LINE__,pathToConfig);
        exit(1);
    }
    else
    {
        //do nothing
    }

    //get number of nodes
    while (-1 != (read = getline(&line, &len, fp))) 
    {
        char* str = removeLeadingTrailingWhitespace(line);
        
        //check if valid line
        if (1 == isValidLine(str))
        {
            numNodes = atoi(str);
            break;
        }
        else
        {
            continue;
        }

        free(str);
        str = NULL;
    }

    //Parse file for La lines
    k = 0;
    while (-1 != (read = getline(&line, &len, fp))) 
    {

        char* str = removeLeadingTrailingWhitespace(line);

        //check if valid line
        if (1 == isValidLine(str))
        {
            char* uid = strtok(str," ");
            char* hostName = strtok(NULL," ");
            char* listeningPort = strtok(NULL," ");

            if (atoi(uid) == myUID)
            {

                node.myUID = atoi(uid);
                strcpy(node.myHostName,hostName);
                node.myListeningPort = atoi(listeningPort);
            }
            else
            {
                //do nothing
            }

            //save information on all nodes for later when assigning edges
            nodeUIDs[k] = atoi(uid);
            strcpy(nodeHostNames[k],hostName);
	        nodeListeningPorts[k] = atoi(listeningPort);

            k=k+1;
            if (k >= numNodes)
            {
                break;
            }
            else
            {
                //do nothing
            }
        }
        else
        {
            continue;
        }

        free(str);
        str = NULL;
    }

    int l = 0;
    //Parse file for Lb lines
    while (-1 != (read = getline(&line, &len, fp))) 
    {
        char* str = removeLeadingTrailingWhitespace(line);
        char UID1_str[20];
        char UID2_str[20];
        char* weight_str;
        
        //check if valid line
        if (1 == isValidLine(str))
        { 

            char *startChar, *middleChar, *endChar;
            int startIdx, middleIdx, endIdx;

            if((NULL != (startChar = strchr(str, '('))) 
                && (NULL != (middleChar = strchr(str,','))) 
                    && (NULL != (endChar = strchr(str,')'))))
            {
                startIdx = (int)(startChar - str);
                middleIdx = (int)(middleChar - str);
                endIdx = (int)(endChar - str);

                memset(UID1_str,'\0',sizeof(UID1_str));
                memset(UID2_str,'\0',sizeof(UID2_str));
                strncpy(UID1_str, str + startIdx + 1, (middleIdx - startIdx - 1));
                strncpy(UID2_str, str + middleIdx + 1,(endIdx - middleIdx - 1));

                strtok(str,") ");
                weight_str = strtok(NULL,") ");

                int UID1 = atoi(UID1_str);
                int UID2 = atoi(UID2_str);
                int weight = atoi(weight_str);

                //printf("<%s,%s,%d>\tnode.myUID = %d, UID1 = %d, UID2 = %d, weight = %d\n",__FILE__,__func__,__LINE__,node.myUID,UID1,UID2,weight);

                if(UID1 == node.myUID)
                {
                    int i;
                    for(i = 0; i < numNodes; i++)
                    {
                        if(UID2 == nodeUIDs[i])
                        {
                            node.neighbourUIDs[l] = UID2;
                            strcpy(node.neighbourHostNames[l],nodeHostNames[i]);
                            node.neighbourListeningPorts[l] = nodeListeningPorts[i];
                            node.neighbourEdgeWeights[l] = weight;
                            l = l + 1;
                            break;
                        }
                    }
                }
                else if(UID2 == node.myUID)
                {

                    int i;
                    for(i = 0; i < numNodes; i++)
                    {
                        if(UID1 == nodeUIDs[i])
                        {
                            node.neighbourUIDs[l] = UID1;
                            strcpy(node.neighbourHostNames[l],nodeHostNames[i]);
                            node.neighbourListeningPorts[l] = nodeListeningPorts[i];
                            node.neighbourEdgeWeights[l] = weight;
                            l = l + 1;
                            break;
                        }
                    }
                }
                else
                {
                    //do nothing
                }
            }
            else
            {
                //this must be the line with the distinguished node
                int distinguishedUID = atoi(str);

                //printf("<%s,%s,%d>\tnode.myUID = %d, distinguishedUID = %s\n",__FILE__,__func__,__LINE__,node.myUID,distinguishedUID);
                if(node.myUID == distinguishedUID)
                {
                    node.isDistinguished = DISINGUISHED;
                }
            }
        }
        else
        {
            continue;
        }
    }

    node.numNeighbours = l;

    fclose(fp);

    return node;
}

char* removeLeadingTrailingWhitespace(char* line)
{
 	int idx = 0, i,j,k = 0;
 
    while (line[idx] == ' ' || line[idx] == '\t' || line[idx] == '\n')
    {
        idx++;
    }

    char* temp = (char*) malloc(sizeof(char) * strlen(line) + 1);

    for(j = idx, i = 0; j < strlen(line); i++, j++)
    {
        temp[i] = line[j];
    }

    temp[j+1] = '\0';

    idx = (int)(strchr(temp, '\n') - temp);
    if (idx > 0)
    {
        temp[idx] = '\0';
    }
   
    return temp;
}

int isValidLine(char* line)
{
    if(0 == strcmp(line,"\n"))
        return 0;

    if(NULL != strstr(line,"#"))
        return 0;

    return 1;
}