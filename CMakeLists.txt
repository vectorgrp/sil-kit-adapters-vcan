cmake_minimum_required(VERSION 3.13)
project(VectorSilKitAdapterVcan)

set(SILKIT_ADAPTER_VCAN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(SILKIT_ADAPTER_VCAN_LIBRARY_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

include(common/cmake/RepoTargets.cmake)
include(common/cmake/Building.cmake)

add_subdirectory(SocketCAN/adapter)
add_subdirectory(SocketCAN/demos)

################################################################################
# Distribution of the source code and binaries
################################################################################
# Install sources and binaries
# Copy all files from the source and bin directory to the proper destination
# Leave out git repo related data

include(common/cmake/InstallFunctions.cmake)

install_adapter_source(
    ${CMAKE_CURRENT_SOURCE_DIR}/SocketCAN
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/QEMU
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/.devcontainer
    ${CMAKE_CURRENT_SOURCE_DIR}/.vscode
)

install_root_source_files()

install_scripts_to(SocketCAN/demos/shell_scripts
    ${CMAKE_CURRENT_SOURCE_DIR}/SocketCAN/demos/shell_scripts/setup_vcan_device.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/SocketCAN/demos/shell_scripts/send_CAN_frames.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/SocketCAN/demos/shell_scripts/setup_vcan_start_adapter_send_CAN_frames.sh
)

install_scripts_to(SocketCAN/demos/shell_scripts/tasks_scripts
    ${CMAKE_CURRENT_SOURCE_DIR}/SocketCAN/demos/shell_scripts/tasks_scripts/build_adapter.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/SocketCAN/demos/shell_scripts/tasks_scripts/prepare_run_silkit.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/SocketCAN/demos/shell_scripts/tasks_scripts/run_EchoDevice.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/SocketCAN/demos/shell_scripts/tasks_scripts/stop_processes.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/SocketCAN/demos/shell_scripts/tasks_scripts/traffic_monitoring.sh
)

install_scripts_to(SocketCAN/demos/shell_scripts/Cuttlefish
    ${CMAKE_CURRENT_SOURCE_DIR}/SocketCAN/demos/shell_scripts/Cuttlefish/prepare_cuttlefish_vcan_device.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/SocketCAN/demos/shell_scripts/Cuttlefish/send_CAN_frames.sh
)

install_scripts_to(SocketCAN/demos/CANoe4SW_SE
    ${CMAKE_CURRENT_SOURCE_DIR}/SocketCAN/demos/CANoe4SW_SE/run.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/SocketCAN/demos/CANoe4SW_SE/run_all.sh
)

include(GNUInstallDirs)

install_silkit_lib(
    ${SILKIT_ADAPTER_VCAN_OUTPUT_DIRECTORY}
    ${SILKIT_ADAPTER_VCAN_LIBRARY_DIRECTORY}
)

install_demo(sil-kit-demo-can-echo-device)

# only sil-kit-adapter-vcan should be installed to /usr/local/bin by calling --target install (therefore it is not excluded)
install_adapter(sil-kit-adapter-vcan)

###############################################################################
# Packaging
###############################################################################
include(common/cmake/Packaging.cmake)

declare_package(
  "SilKit-Adapter-vcan"
  "binary release of SIL Kit Adapter vcan"
)