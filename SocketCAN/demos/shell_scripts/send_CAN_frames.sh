#!/bin/bash

# Check if the can_device argument is provided
if [[ -z $1 ]]; then
    echo "[error] CAN device name must be provided."
    exit 1
fi
can_device=$1

N_FRAMES=300
CAN_FD_PAYLOAD="AA.AA.AA.AA.BB.BB.BB.BB.CC.CC.CC.DD" 
CAN_PAYLOAD="AA.AA.BB.BB"
CAN_FD_ID="005"
CAN_ID="001"

# Check if the --enable-fd argument is passed
if [[ $2 == "-fd" ]]; then
    echo "[info] Starting sending CAN FD frames on [$can_device]..."
    for (( i = 1; i <= $N_FRAMES ; i++ ))
    do
        # Send CAN FD frame with ID 005 and 64 bytes of data
        cansend $can_device $CAN_FD_ID##0.$CAN_FD_PAYLOAD
        echo "Frame $i : {"$CAN_FD_ID##0.$CAN_FD_PAYLOAD"} sent to [$can_device]"
        sleep 0.5
    done
    echo "[info] Sending CAN FD frames ended."
else
    echo "[info] Starting sending CAN frames on [$can_device]..."
    for (( i = 1; i <= $N_FRAMES ; i++ ))
    do
        # Send CAN frame with ID 001 and 8 bytes of data
        cansend $can_device $CAN_ID#$CAN_PAYLOAD
        echo "Frame $i : {$CAN_ID#$CAN_PAYLOAD} sent to [$can_device]"
        sleep 0.5
    done
    echo "[info] Sending CAN frames ended."
fi

echo "[info] Restart the script if you want to generate more CAN traffic."
