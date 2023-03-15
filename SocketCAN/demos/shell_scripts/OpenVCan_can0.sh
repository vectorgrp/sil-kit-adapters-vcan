#!/bin/bash
echo "Loading SocketCAN kernel modules"
sudo modprobe vcan
echo "Creating Virtual CAN interface with name [can0]"
sudo ip link add dev can0 type vcan
sudo ip link set up can0
echo "Verify interface is up & running"
ip link show can0
