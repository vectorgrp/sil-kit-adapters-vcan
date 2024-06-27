#!/bin/bash
scriptDir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

silKitDir=/home/vector/vfs/SILKit/SilKit-4.0.50-ubuntu-18.04-x86_64-gcc/

# if "exported_full_path_to_silkit" environment variable is set (in pipeline script), use it. Otherwise, use default value
silKitDir="${exported_full_path_to_silkit:-$silKitDir}"

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
$silKitDir/SilKit/bin/sil-kit-registry --listen-uri 'silkit://0.0.0.0:8501' -s &> $scriptDir/sil-kit-registry.out &
sleep 1 # wait 1 second for the creation/existense of the .out file
timeout 30s grep -q 'Registered signal handler' <(tail -f /$scriptDir/sil-kit-registry.out) || { echo "[error] Timeout reached while waiting for sil-kit-registry to start"; exit 1; }

# Start the SilKitDemoCanEchoDevice
$scriptDir/../../../bin/sil-kit-demo-can-echo-device &> $scriptDir/sil-kit-demo-can-echo-device.out &
timeout 30s grep -q 'Press CTRL + C to stop the process...' <(tail -f /$scriptDir/sil-kit-demo-can-echo-device.out) || { echo "[error] Timeout reached while waiting for sil-kit-demo-can-echo-device to start"; exit 1; }

# Run and test adapter with vCAN device of MTU 72
$scriptDir/../shell_scripts/setup_vCAN_start_adapter_send_CAN_frames.sh &> $scriptDir/setup_vCAN_start_adapter_send_CAN_frames.out &
script_pid=$!


$scriptDir/run.sh
#capture returned value of run_mtu72.sh script
exit_status=$?

if [[ exit_status -ne 0 ]]; then
  exit $exit_status
fi

kill $script_pid

# Run and test adapter with vCAN device of MTU 16
$scriptDir/../shell_scripts/setup_vCAN_start_adapter_send_CAN_frames.sh -mtu16 &> $scriptDir/setup_vCAN_start_adapter_send_CAN_frames-mtu16.out &
$scriptDir/run.sh -mtu16
#capture returned value of run_mtu16.sh script
exit_status=$?

if [[ exit_status -ne 0 ]]; then
  exit $exit_status
fi

echo "sil-kit-registry.out:--------------------------------------------------------------------------------------" 
cat $scriptDir/sil-kit-registry.out
echo "-----------------------------------------------------------------------------------------------------------" 

echo "setup_vCAN_start_adapter_send_CAN_frames.out:--------------------------------------------------------------" 
cat $scriptDir/setup_vCAN_start_adapter_send_CAN_frames.out
echo "-----------------------------------------------------------------------------------------------------------" 

echo "setup_vCAN_start_adapter_send_CAN_frames-mtu16.out:--------------------------------------------------------"
cat $scriptDir/setup_vCAN_start_adapter_send_CAN_frames-mtu16.out
echo "-----------------------------------------------------------------------------------------------------------"

#exit run_all.sh with same exit_status
exit $exit_status

