#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ConfigParser.h"

int isValidLine(char* line);

char* removeLeadingTrailingWhitespace(char* line);

struct Node Parse(char* machineName, char* pathToConfig)
{
    struct Node nodeInfo;

    FILE *fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    char nodeUID[MAX_NEIGHBOURS][CHAR_BUFFER_SIZE];
    char nodeHostName[MAX_NEIGHBOURS][CHAR_BUFFER_SIZE];
	char nodeListeningPort[MAX_NEIGHBOURS][CHAR_BUFFER_SIZE];

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

            if (0 == strcmp(machineName,hostName))
            {
                strcpy(nodeInfo.myUID,uid);
                strcpy(nodeInfo.myHostName,hostName);
                strcpy(nodeInfo.myListeningPort,listeningPort);

                hostK = k;
            }
            else
            {
                //do nothing
            }

            strcpy(nodeUID[k],uid);
            strcpy(nodeHostName[k],hostName);
            strcpy(nodeListeningPort[k],listeningPort);

            k++;
            if(k >= numNodes)
                break;
        }
        else
        {
            continue;
        }

        free(str);
        str = NULL;
    }

    //Parse file for Lb lines
    k = 0;
    while (-1 != (read = getline(&line, &len, fp))) 
    {
        char* str = removeLeadingTrailingWhitespace(line);
        char* uid = NULL;
        
        int l = 0;

        //check if valid line
        if (1 == isValidLine(str))
        {  
            if(k == hostK)
            {
                uid = strtok(str," ");

                strcpy(nodeInfo.neighbourUIDs[l],uid);
                nodeInfo.numNeighbours = 1;
                l++;

                while(NULL != (uid = strtok(NULL," ")))
                {
                    strcpy(nodeInfo.neighbourUIDs[l],uid);
                    nodeInfo.numNeighbours++;
                    l++;
                }

                free(str);
                str = NULL;
                break;
            }
            else
            {

            }
        }
        else
        {
            continue;
        }

        k++;
    }

	int i,j,l=0;
    for(j = 0; j < nodeInfo.numNeighbours; j++)
    {
	    for(i = 0 ; i < numNodes; i++)
	    {
            if(0 == strcmp(nodeUID[i],nodeInfo.neighbourUIDs[j]))
            {
                strcpy(nodeInfo.neighbourUIDs[l],nodeUID[i]);
                strcpy(nodeInfo.neighbourHostNames[l],nodeHostName[i]);
                strcpy(nodeInfo.neighbourListeningPorts[l],nodeListeningPort[i]);
                l++;
            }
        }
	}

    fclose(fp);

    return nodeInfo;
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
    if(isdigit(line[0]))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}