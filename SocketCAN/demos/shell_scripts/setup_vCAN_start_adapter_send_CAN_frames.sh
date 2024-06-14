#!/bin/bash

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)

# cleanup trap for child processes 
trap 'kill $(jobs -p); exit' EXIT SIGHUP;

# check if user is root
if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root / via sudo!"
    exit 1
fi

# Setup an FD-capable vCAN device (default case)
echo "Setting up [can0]..."
$SCRIPT_DIR/setup_vCAN_device.sh can0 &> $SCRIPT_DIR/setup_vCAN_device_can0.out

# Start adapter 
echo "Starting SilKitAdapterSocketCAN..."
$SCRIPT_DIR/../../../bin/SilKitAdapterSocketCAN &> $SCRIPT_DIR/SilKitAdapterSocketCAN.out &
adapter_pid=$!

sleep 1 # wait 1 second for the creation/existence of the .out file

timeout 30s grep -q 'Press CTRL + C to stop the process...' <(tail -f /$SCRIPT_DIR/SilKitAdapterSocketCAN.out) || { echo "[error] Timeout reached while waiting for sil-kit-registry to start"; exit 1; }
echo "SilKitAdapterSocketCAN has been started"

# Send frames on [can0] 
echo "Sending Classical CAN frames on [can0]..."
$SCRIPT_DIR/send_CAN_frames.sh can0 &> $SCRIPT_DIR/send_ClassicalCAN_frames.out &
echo "Sending CAN FD frames on [can0]..."
$SCRIPT_DIR/send_CAN_frames.sh can0 -fd &> $SCRIPT_DIR/send_CANFD_frames.out &

# Wait for the SendSocketCANFrames process to complete (300 frames @ 2Hz -> 150 sec maximum wait)
sleep 15
