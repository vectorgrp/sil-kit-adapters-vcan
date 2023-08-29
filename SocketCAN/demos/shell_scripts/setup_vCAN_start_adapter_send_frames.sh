#!/bin/bash

scriptDir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)

# cleanup trap for child processes 
trap 'kill $(jobs -p); exit' EXIT SIGHUP;

# check if user is root
if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root / via sudo!"
    exit 1
fi


# Make a temporary fifo to use as std:cin which is not fd#0 (this shell's std:cin) to prevent unintended closure of the background-launched processes
tmp_fifo=$(mktemp -u)
mkfifo $tmp_fifo
exec 3<>$tmp_fifo
rm $tmp_fifo

# Setup can0 
echo "Setting up [can0]..."
<&3 $scriptDir/OpenVCan_can0.sh &> $scriptDir/OpenVCan_can0.out


# Start adapter 
echo "Starting SKA vCAN..."
<&3 $scriptDir/../../../build/bin/SilKitAdapterSocketCAN &> $scriptDir/SilKitAdapterSocketCAN.out &

# Wait for adapter to start and connect to both [can0] and SIL Kit
sleep 2

# Send frames on [can0] 
echo "Sending CAN frames on [can0]..."
<&3 $scriptDir/SendSocketCANFrames.sh &> $scriptDir/SendSocketCANFrames.out &

# Wait for the SendSocketCANFrames process to complete (300 frames @ 2Hz -> 150 sec maximum wait)
sleep 150
