#!/bin/bash

for i in {300..400}
do
cansend can0 $i#AAAABBBB
sleep 0.25
done
