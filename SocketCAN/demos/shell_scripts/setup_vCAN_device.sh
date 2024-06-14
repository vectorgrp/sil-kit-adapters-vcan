#!/bin/bash

# This script is uniquely used for testing and demo purposes of the adapter
# It takes a mandatory <vCAN device name> argument and an optional [-mtu16] flag
# If the [-mtu16] flag is provided, the resulting vCAN device will have a small MTU, for testing purposes

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)

# Check if user is root
if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root / via sudo!"
    exit 1
fi

# Check if the correct number of arguments is provided
if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <vcan_device_name> [-mtu16]"
    exit 1
fi

vcan_device_name=$1
mtu_value=72 # default value

if [[ $# -gt 1 ]]; then
    if [[ $2 == "-mtu16" ]]; then
        mtu_value=16
    else
        echo "Invalid argument. Use -mtu16 to set the MTU to 16."
        exit 1
    fi
fi

echo "Loading vCAN kernel module"
modprobe vcan

# If interface exists and is up, delete it
if ip link show $vcan_device_name | grep -q "<NOARP,UP,LOWER_UP>" ; then
  	echo "[$vcan_device_name] interface is already exists. Deleting..."
	ip link set down $vcan_device_name
	ip link delete $vcan_device_name type vcan
	echo "[$vcan_device_name] deleted."
fi

# Setup a fresh vCAN device with the specified args
echo "Setting up [$vcan_device_name] with MTU=$mtu_value"
ip link add dev $vcan_device_name type vcan
ip link set $vcan_device_name mtu $mtu_value
ip link set up $vcan_device_name

# Make sure all is OK
if ip link show $vcan_device_name | grep -q "<NOARP,UP,LOWER_UP>"; then
        echo "vCAN interface [$vcan_device_name] is up & running."
else
        echo "Something is wrong with vCAN interface [$vcan_device_name]"
fi
