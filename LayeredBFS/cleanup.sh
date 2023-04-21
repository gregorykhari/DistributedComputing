#!/bin/bash

# Change this to your netid
netid=gkh210000

# Root directory of your project
PROJECT_DIR=$HOME/TestProject

# Directory where the config file is located on your local system
#Change the configfile location in this!
CONFIG_LOCAL=/Users/gregoryhinkson/Projects/DistributedComputing/LayeredBFS/configfile.txt

n=0

# shellcheck disable=SC2002
cat "$CONFIG_LOCAL" | sed -e "s/#.*//" | sed -e "/^\s*$/d" |
(
    # shellcheck disable=SC2162
    read i
    echo "$i"
    while [[ $n -lt $i ]]
    do
    	read line
        # shellcheck disable=SC2086
        host=$( echo $line | awk '{ print $2 }' )

        echo "$host"
        osascript -e '
                tell app "Terminal"
                    do script "ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no '$netid@$host'  killall -u '$netid'"
                end tell'
        sleep 0.2

        n=$(( n + 1 ))
    done
   
)


echo "Cleanup complete"
