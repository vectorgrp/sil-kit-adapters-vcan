#!/bin/bash

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)

# cleanup trap for child processes 
trap 'kill $(jobs -p); exit' EXIT SIGHUP;

# check if user is root
if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root / via sudo!"
    exit 1
fi

# Setup can0 
echo "Setting up [can0]..."
$SCRIPT_DIR/OpenVCan_can0.sh &> $SCRIPT_DIR/OpenVCan_can0.out


# Start adapter 
echo "Starting SilKitAdapterSocketCAN..."
$SCRIPT_DIR/../../../bin/SilKitAdapterSocketCAN &> $SCRIPT_DIR/SilKitAdapterSocketCAN.out &
sleep 1 # wait 1 second for the creation/existens of the .out file

timeout 30s grep -q 'Press CTRL + C to stop the process...' <(tail -f /$SCRIPT_DIR/SilKitAdapterSocketCAN.out) || exit 1
echo "SilKitAdapterSocketCAN has been started"

# Send frames on [can0] 
echo "Sending CAN frames on [can0]..."
$SCRIPT_DIR/SendSocketCANFrames.sh &> $SCRIPT_DIR/SendSocketCANFrames.out &

# Wait for the SendSocketCANFrames process to complete (300 frames @ 2Hz -> 150 sec maximum wait)
sleep 150
