***************************** README *********************************

Name: Gregory Hinkson & Utsav Adhikari
UTD NETID: GKH210000 & UXA200002
Assignment: CS6380 Project 1


Files:
	1) PelegsAlgorithm.c - the main program which executes PelegsAlgorithm for Leader election followed by BFS
	2) Node.h - defines data about node necessary for execution of Pelegs algorithm and BFS 
	3) Node.c - contains functions to display information about the node
    4) ConfigParser.h - contains definition of function to parse valid configfiles
    5) ConfigParser.c - contains Parse function definition to parse valid configfiles
    6) makefile - makefile to compile PelegsAlgorithm.c
	7) configfile.txt - a valid configfile containing the number of nodes
    8) launcher.sh - a bash script to automatically execute PelegsAlgorithm and BFS across nodes within configfile

How to run:
    1) Manually
        * open N terminals (one on each Node)
        * For each terminal, SSH into one of the machines outlined in the configfile.txt
        * in one of the N terminals execute:
            > make
        * in all N terminals, in any order, execute:
            > ./Pelegs.o -i <Path/to/Configfile>

    2) Automatically
        * open a terminal and execute:
            > scp * <UTD_ID>@dcXX.utdallas.edu:<PATH_TO_PROJECT_FILES_ON_dcXX_MACHINE>
            - in order to copy all files over to dcXX machine where XX is valid machine
        * SSH into one of the machines and naviate to <PATH_TO_PROJECT_FILES_ON_dcXX_MACHINE>, then execute:
            > make
        * Open launcher.sh and edit as follows:
            - edit line 4 "netid=" to be valid netid
            - edit linen 11 "CONFIG_LOCAL=" to be path of config file on local machine
            - edit line 28:
                do script "ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no '$netid@$host' ./<PATH_TO_PROJECT_FILES_ON_dcXX_MACHINE>/Pelegs.o -i <PATH_TO_FILES_ON_dcXX_MACHINE>/configfile.txt"


Notes:
    1) To run automatically, passwordless login must be setup for dcXX machines
	
