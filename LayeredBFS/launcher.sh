#!/bin/bash

# Change this to your netid
netid=gkh210000

# Root directory of your project
PROJECT_DIR=$HOME/TestProject

# Directory where the config file is located on your local system
#Change the configfile location in this!
CONFIG_LOCAL=/Users/gregoryhinkson/Projects/DistributedComputing/LayeredBFS/configfile.txt

n=0

cat $CONFIG_LOCAL | sed -e "s/#.*//" | sed -e "/^\s*$/d" |
(
    read i
    echo $i
    while [[ $n < $i ]]
    do
    	read line
    	p=$( echo $line | awk '{ print $1 }' )
        host=$( echo $line | awk '{ print $2 }' )
	
	#gnome-terminal --bash -c "ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $netid@$host ./../github/PelegsAlgorithm/a.out; exec bash -i" &
    osascript -e '
                tell app "Terminal"
                    do script "ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no '$netid@$host' stdbuf -o0 ./Courses/CS6380-DC/Project3/LayeredBFS.o -i /home/010/g/gk/gkh210000/Courses/CS6380-DC/Project3/configfile.txt -u '$p' "
                end tell'
    
        n=$(( n + 1 ))
    done
)
