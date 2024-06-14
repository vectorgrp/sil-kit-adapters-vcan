param (
    [switch]$remote
)

#configure error handling
$ErrorActionPreference = "Stop"
Set-StrictMode -Version 3

#get folder of CANoe4SW Server Edition
$canoe4sw_se_install_dir = $env:CANoe4SWSE_InstallDir64

#create environment
& $canoe4sw_se_install_dir/environment-make.exe "$PSScriptRoot/venvironment.yaml"  -o "$PSScriptRoot" -A "Linux64"

#compile test unit
& $canoe4sw_se_install_dir/test-unit-make.exe "$PSScriptRoot/../tests/test_CANFD_EchoDevice.vtestunit.yaml" -e "$PSScriptRoot/Default.venvironment" -o "$PSScriptRoot"

if ($remote) 
{
	# remote machine and paths configuration  
	$remote_username="vector"
	$remote_ip="192.168.56.102"
	$remote_SKA_base_path="/home/$remote_username/vfs/sil-kit-adapters-vcan"
	$artifacts_subdirectory = "SocketCAN/demos/CANoe4SW_SE"
	$remote_full_path = Join-Path -Path $remote_SKA_base_path -ChildPath $artifacts_subdirectory

	#copy artifacts to VM
	Write-Host "Copying artifacts to [${remote_username}@${remote_ip}:${remote_full_path}]..."
	scp -r $PSScriptRoot/Default.venvironment $PSScriptRoot/test_CANFD_EchoDevice.vtestunit "${remote_username}@${remote_ip}:${remote_full_path}"
	Write-Host "Done."
}

