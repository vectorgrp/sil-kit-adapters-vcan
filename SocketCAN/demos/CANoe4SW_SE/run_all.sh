#!/bin/bash
# SPDX-FileCopyrightText: Copyright 2025 Vector Informatik GmbH
# SPDX-License-Identifier: MIT

scriptDir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
silKitDir=/home/vector/SilKit/SilKit-5.0.2-ubuntu-22.04-x86_64-gcc/
# if "exported_full_path_to_silkit" environment variable is set (in pipeline script), use it. Otherwise, use default value
silKitDir="${exported_full_path_to_silkit:-$silKitDir}"

# Ensure the logs directory exists
logDir=$scriptDir/logs # define a directory for .out files
mkdir -p $logDir # if it does not exist, create it

# create a timestamp for log files
timestamp=$(date +"%Y%m%d_%H%M%S")

# cleanup trap for child processes 
trap 'kill $(jobs -p); exit' EXIT SIGHUP;

if [ ! -d "$silKitDir" ]; then
    echo "The var 'silKitDir' needs to be set to actual location of your SilKit"
    exit 1
fi

# check if user is root
if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root / via sudo!"
    exit 1
fi

# Start the sil-kit-registry
$silKitDir/SilKit/bin/sil-kit-registry --listen-uri 'silkit://0.0.0.0:8501' &>  $logDir/sil-kit-registry_${timestamp}.out &
sleep 1 # wait 1 second for the creation of the .out file
timeout 30s grep -q 'Press Ctrl-C to terminate...' <(tail -f $logDir/sil-kit-registry_${timestamp}.out -n +1) || { echo "[error] Timeout reached while waiting for sil-kit-registry to start"; exit 1; }

# Start the SilKitDemoCanEchoDevice
$scriptDir/../../../bin/sil-kit-demo-can-echo-device &> $logDir/sil-kit-demo-can-echo-device_${timestamp}.out &
sleep 1 # wait 1 second for the creation of the .out file
timeout 30s grep -q 'Press CTRL + C to stop the process...' <(tail -f $logDir/sil-kit-demo-can-echo-device_${timestamp}.out -n +1) || { echo "[error] Timeout reached while waiting for sil-kit-demo-can-echo-device to start"; exit 1; }

# Run and test adapter with vcan device of MTU 72
$scriptDir/../shell_scripts/setup_vcan_start_adapter_send_CAN_frames.sh &> $logDir/setup_vcan_start_adapter_send_CAN_frames_${timestamp}.out &
script_pid=$!
sleep 1 # wait 1 second for the creation of the .out file
timeout 90s grep -q 'Adapter has been launched and CAN payload is being generated...' <(tail -f $logDir/setup_vcan_start_adapter_send_CAN_frames_${timestamp}.out -n +1) || { echo "[error] Timeout reached while waiting for (setup_vcan_start_adapter_send_CAN_frames.sh) to start"; exit 1; }

$scriptDir/run.sh
exit_status=$?

if [[ exit_status -ne 0 ]]; then
  exit $exit_status
fi

kill $script_pid

# Run and test adapter with vcan device of MTU 16
$scriptDir/../shell_scripts/setup_vcan_start_adapter_send_CAN_frames.sh -mtu16 &> $logDir/setup_vcan_start_adapter_send_CAN_frames-mtu16_${timestamp}.out &
sleep 1 # wait 1 second for the creation of the .out file
timeout 60s grep -q 'Adapter has been launched and CAN payload is being generated...' <(tail -f $logDir/setup_vcan_start_adapter_send_CAN_frames-mtu16_${timestamp}.out -n +1) || { echo "[error] Timeout reached while waiting for (setup_vcan_start_adapter_send_CAN_frames.sh -mtu16) to start"; exit 1; }

$scriptDir/run.sh -mtu16

exit $?