#!/bin/sh

# Load the vcan kernel modules
echo "[info] loading kernel modules"
insmod /system_dlkm/lib/modules/vcan.ko
insmod /system_dlkm/lib/modules/can.ko
insmod /system_dlkm/lib/modules/can-raw.ko

# Create the vcan device
echo "[info] creating (can0) vcan device "
ip link add dev can0 type vcan

# Bring up the vcan device
ip link set up can0

# Show the status of the vcan device
ip link show can0