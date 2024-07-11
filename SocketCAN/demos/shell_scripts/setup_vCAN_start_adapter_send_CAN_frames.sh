#!/bin/bash

scriptDir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)

logDir=$scriptDir/../CANoe4SW_SE/logs # define a directory for .out files
mkdir -p $logDir # if it does not exist, create it

# cleanup trap for child processes 
trap 'kill $(jobs -p); ip link set down can0; ip link delete can0 type vcan; exit' EXIT SIGHUP;

# check if user is root
if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root / via sudo!"
    exit 1
fi

if [[ $1 == "-mtu16" || -z $1 ]]; then
  arg=$1
else
  echo "[Error] Invalid argument. Use -mtu16 to set the MTU to 16."
  exit 1
fi

# Setup an FD-capable vCAN device (default case)
echo "[info] Setting up [can0]..."
$scriptDir/setup_vCAN_device.sh can0 $arg &> $logDir/setup_vCAN_device_can0$arg.out

# Start adapter 
echo "[info] Starting sil-kit-adapter-vcan..."
$scriptDir/../../../bin/sil-kit-adapter-vcan --name SilKitAdapterSocketCAN$arg &> $logDir/sil-kit-adapter-vcan$arg.out &
sleep 1 # wait 1 second for the creation/existence of the .out file
timeout 30s grep -q 'Press CTRL + C to stop the process...' <(tail -f $logDir/sil-kit-adapter-vcan$arg.out -n +1) || { echo "[error] Timeout reached while waiting for sil-kit-adapter-vcan to start"; exit 1; }
echo "[info] sil-kit-adapter-vcan has been started"

echo "[info] Sending Classical CAN frames on [can0]..."
$scriptDir/send_CAN_frames.sh can0 &> $logDir/send_ClassicalCAN_frames.out &
sleep 1
timeout 30s grep -q '\{001#AA.AA.BB.BB\} sent to \[can0\]' <(tail -f $logDir/send_ClassicalCAN_frames.out -n +1) || { echo "[error] Timeout reached while waiting for (send_CAN_frames.sh can0) to start"; exit 1; }

# Only send CANFD frames if vCAN device is compatible (not -mtu arg is passed to script)
if [[ -z $arg ]]; then
  echo "[info] Sending CAN FD frames on [can0]..."
  $scriptDir/send_CAN_frames.sh can0 -fd &> $logDir/send_CANFD_frames.out &
  sleep 1
  timeout 30s grep -q '\{005##0.AA.AA.AA.AA.BB.BB.BB.BB.CC.CC.CC.DD\} sent to \[can0\]' <(tail -f $logDir/send_CANFD_frames.out -n +1) || { echo "[error] Timeout reached while waiting for (send_CAN_frames.sh can0 -fd) to start"; exit 1; }
fi

echo "[info] Adapter has been launched and CAN payload is being generated..."
sleep 15