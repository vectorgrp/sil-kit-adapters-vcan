#!/bin/sh
# SPDX-FileCopyrightText: Copyright 2025 Vector Informatik GmbH
# SPDX-License-Identifier: MIT

# Check if CAN_UTILS_BUILD_PATH is set and not empty
if [ -z "${CAN_UTILS_BUILD_PATH}" ]; then
  echo "[error] CAN_UTILS_BUILD_PATH is not set. Please export CAN_UTILS_BUILD_PATH before running this script."
  exit 1
# Check if CAN_UTILS_BUILD_PATH points to a valid directory and contains candump and cansend
elif [ ! -d "${CAN_UTILS_BUILD_PATH}" ]; then
  echo "[error] CAN_UTILS_BUILD_PATH does not point to a valid directory. Please check the path."
  exit 1
# Check if cansend is available and executable
elif [ ! -x "${CAN_UTILS_BUILD_PATH}/cansend" ] || [ ! -x "${CAN_UTILS_BUILD_PATH}/candump" ]; then
  echo "[error] necessary can-utils are not found or not executable in ${CAN_UTILS_BUILD_PATH}. Please verify their availability."
  exit 1
fi

echo "CAN_UTILS_BUILD_PATH is set and valid: ${CAN_UTILS_BUILD_PATH}"

# Check if the can_device argument is provided
if [ -z "$1" ]; then
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
if [ "$2" = "-fd" ]; then
    echo "[info] Starting sending CAN FD frames on [$can_device]..."
    i=1
    while [ $i -le $N_FRAMES ]
    do
        # Send CAN FD frame with ID 005 and 64 bytes of data
        $CAN_UTILS_BUILD_PATH/cansend $can_device $CAN_FD_ID##0.$CAN_FD_PAYLOAD
        echo "Frame $i : {"$CAN_FD_ID##0.$CAN_FD_PAYLOAD"} sent to [$can_device]"
        i=$((i + 1))
        sleep 0.5
    done
    echo "[info] Sending CAN FD frames ended."
else
    echo "[info] Starting sending CAN frames on [$can_device]..."
    i=1
    while [ $i -le $N_FRAMES ]
    do
        # Send CAN frame with ID 001 and 8 bytes of data
        $CAN_UTILS_BUILD_PATH/cansend $can_device $CAN_ID#$CAN_PAYLOAD
        echo "Frame $i : {$CAN_ID#$CAN_PAYLOAD} sent to [$can_device]"
        i=$((i + 1))
        sleep 0.5
    done
    echo "[info] Sending CAN frames ended."
fi

echo "[info] Restart the script if you want to generate more CAN traffic."
