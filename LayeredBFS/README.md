***************************** README *********************************

Name: Gregory Hinkson & Utsav Adhikari
UTD NETID: GKH210000 & UXA200002
Assignment: CS6380 Project 3

Files:
1. LayeredBFS.c - the main program which executes LayeredBFS for generating BFS Tree in asynchronous Network.
2. Node.h - defines data about node necessary for execution of LayeredBFS
3. Node.c - contains functions to display information about the node
4. Message.h - contains definition of message structure for TCP/IP messages between nodes
5. Message.c - contains functions to create a message and print the details of the message
6. MessageQueue.h - contains definition of message queue to buffer the incoming messages
7. ConfigParser.h - contains definition of function to parse valid configfiles
8. ConfigParser.c - contains Parse function definition to parse valid configfiles
9. Connection.c - contains functions to establish connection with neighbouring nodes 
10. Connection.h - contains definitions to establish connection with other nodes
11. makefile - makefile to compile LayeredBFS.c
12. configfile.txt - a valid configfile containing the number of nodes
13. launcher.sh - a bash script to automatically execute LayeredBFS with the configuration given in configfile.txt
14. cleanup.sh - a bash script to automatically kills processes initiated on dc machines from previous execution for user
15. README.md - contains file definitions and how to execute program

How to run:
1. Manually
    * open N terminals (one on each Node)
    * For each terminal, SSH into one of the machines outlined in the configfile.txt
    * in one of the N terminals execute:
        - make
    * in all N terminals, in any order, execute:
        - ./LayeredBFS.o -i <Path/to/Configfile> -u <UID_of_current_node>

2. Automatically
    * open a terminal and execute:
        - scp * <UTD_ID>@dcXX.utdallas.edu:<PATH_TO_PROJECT_FILES_ON_dcXX_MACHINE>
        - in order to copy all files over to dcXX machine where XX is valid machine
    * SSH into one of the machines and naviate to <PATH_TO_PROJECT_FILES_ON_dcXX_MACHINE>, then execute:
        - make
    * Open launcher.sh and edit as follows:
        - edit line 4 "netid=" to be valid netid
        - edit linen 11 "CONFIG_LOCAL=" to be path of config file on local machine
        - run launcher.sh


Notes:
1. To run automatically, passwordless login must be setup for dcXX machines
	
