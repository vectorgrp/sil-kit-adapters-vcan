#!/bin/bash

echo "Loading SocketCAN kernel modules"

sudo modprobe vcan

echo "Creating Virtual CAN interface with name [can0]"

sudo ip link add dev can0 type vcan

sudo ip link set up can0

echo "Verifying can0 interface state:"

# Check if can0 interface is up : <NOARP,UP,LOWER_UP> flags should be set.

#"NOARP" means that the interface does not use ARP (Address Resolution Protocol) to map IP addresses to hardware addresses

#"UP" means that the interface is currently enabled

#"LOWER_UP" means that the physical link is up

if sudo ip link show can0 | grep -q "<NOARP,UP,LOWER_UP>"

then

        echo "vCAN interface [can0] is up & running."

else

        echo "Something is wrong with vCAN interface [can0]"

fi



