param (
    [switch]$mtu16
)

#configure error handling
$ErrorActionPreference = "Stop"
Set-StrictMode -Version 3

#get folder of CANoe4SW Server Edition
$canoe4sw_se_install_dir = $env:CANoe4SWSE_InstallDir64

#create environment
& $canoe4sw_se_install_dir/environment-make.exe "$PSScriptRoot/venvironment.yaml"  -o "$PSScriptRoot"

if($mtu16)
{
    Write-Host "[Info] Running tests for MTU 16 vCAN devices."
    #compile test unit
    & $canoe4sw_se_install_dir/test-unit-make.exe "$PSScriptRoot/../tests/test_CAN_EchoDevice.vtestunit.yaml" -e "$PSScriptRoot/Default.venvironment" -o "$PSScriptRoot"
    #run test unit
    & $canoe4sw_se_install_dir/canoe4sw-se.exe "$PSScriptRoot/Default.venvironment" -d "$PSScriptRoot/working-dir" --verbosity-level "2" --test-unit "$PSScriptRoot/test_CAN_EchoDevice.vtestunit"  --show-progress "tree-element"
}else{
    Write-Host "[Info] Running tests for MTU 72 vCAN devices."
    #compile test unit
    & $canoe4sw_se_install_dir/test-unit-make.exe "$PSScriptRoot/../tests/test_CANFD_EchoDevice.vtestunit.yaml" -e "$PSScriptRoot/Default.venvironment" -o "$PSScriptRoot"
    #run test unit
    & $canoe4sw_se_install_dir/canoe4sw-se.exe "$PSScriptRoot/Default.venvironment" -d "$PSScriptRoot/working-dir" --verbosity-level "2" --test-unit "$PSScriptRoot/test_CANFD_EchoDevice.vtestunit"  --show-progress "tree-element"
}

