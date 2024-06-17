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

$silKitDir/SilKit/bin/sil-kit-registry --listen-uri 'silkit://0.0.0.0:8501' -s &> $scriptDir/sil-kit-registry.out &
sleep 1 # wait 1 second for the creation/existense of the .out file
timeout 30s grep -q 'Registered signal handler' <(tail -f /$scriptDir/sil-kit-registry.out) || { echo "[error] Timeout reached while waiting for sil-kit-registry to start"; exit 1; }

$scriptDir/../shell_scripts/setup_vCAN_start_adapter_send_CAN_frames.sh &> $scriptDir/setup_vCAN_start_adapter_send_CAN_frames.out &

$scriptDir/../../../bin/SilKitDemoCanEchoDevice &> $scriptDir/SilKitDemoCanEchoDevice.out &

$scriptDir/run.sh

#capture returned value of run.sh script
exit_status=$?

echo "sil-kit-registry.out:--------------------------------------------------------------------------------------" 
cat $scriptDir/sil-kit-registry.out
echo "-----------------------------------------------------------------------------------------------------------" 

echo "setup_vCAN_start_adapter_send_CAN_frames.out:--------------------------------------------------------------" 
cat $scriptDir/setup_vCAN_start_adapter_send_CAN_frames.out
echo "-----------------------------------------------------------------------------------------------------------" 

#exit run_all.sh with same exit_status
exit $exit_status

