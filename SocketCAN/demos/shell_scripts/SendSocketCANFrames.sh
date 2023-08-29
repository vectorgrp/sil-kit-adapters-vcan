#!/bin/bash

echo "Starting sending CAN frames on [can0]..."

#while true
for (( i = 1; i <= 300 ; i++ ))
do
cansend can0 001#AAAABBBB
echo "Frame $i : {001 # AA AA BB BB} sent to [can0]"
sleep 0.5
done

echo "Sending CAN frames ended." 
echo "Restart "SendSocketCANFrames.sh" script if you want to generate more CAN traffic."
