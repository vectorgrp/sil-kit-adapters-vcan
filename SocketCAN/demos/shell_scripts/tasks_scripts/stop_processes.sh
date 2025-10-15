#!/bin/bash
# SPDX-FileCopyrightText: Copyright 2025 Vector Informatik GmbH
# SPDX-License-Identifier: MIT

set -e

echo "[Info] Stopping processes"

# List of processes to be stopped
processes=("traffic_monitoring.sh" "sil-kit-registry" "sil-kit-demo-can-echo-device" "setup_vcan_start_adapter_send_CAN_frames.sh")

# Loop through each process and check if it is running and send SIGINT if it is
for process in "${processes[@]}"; do
    if pgrep -f "$process" > /dev/null; then
        pkill -f "$process"
        echo "[Info] $process has been stopped"
    else
        echo "[Info] $process is not running"
    fi
done
