#!/bin/bash

# check if user is root
if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root / via sudo!"
    exit 1
fi

echo "Loading vCAN kernel module"
modprobe vcan

# Check if [can0] interface is already up: <NOARP,UP,LOWER_UP> flags should be set.
#"NOARP": the interface does not use ARP (Address Resolution Protocol) to map IP addresses to hardware addresses
#"UP": the interface is currently enabled
#"LOWER_UP": the physical link is up
if ip link show can0 | grep -q "<NOARP,UP,LOWER_UP>"; then
	# If can0 interface is up, delete it 
  	echo "[can0] interface is already exists. Deleting..."
	sudo ip link set down can0
	sudo ip link delete can0 type vcan
	echo "[can0] deleted."
fi

# Create a new vcan interface 
echo "Creating [can0] interface."
ip link add dev can0 type vcan
ip link set up can0

# Make sure all is OK
if ip link show can0 | grep -q "<NOARP,UP,LOWER_UP>"; then
        echo "vCAN interface [can0] is up & running."
else
        echo "Something is wrong with vCAN interface [can0]"
fi

