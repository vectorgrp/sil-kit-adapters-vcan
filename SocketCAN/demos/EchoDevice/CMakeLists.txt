# SPDX-FileCopyrightText: Copyright 2025 Vector Informatik GmbH
# SPDX-License-Identifier: MIT
add_executable(sil-kit-demo-can-echo-device
    "SilKitDemoCanEchoDevice.cpp"
)

target_include_directories(sil-kit-demo-can-echo-device 
    PRIVATE
    "${CMAKE_SOURCE_DIR}/SocketCAN/"
)

target_link_libraries(sil-kit-demo-can-echo-device
    PRIVATE
    asio
    sil-kit-adapters-common
    SilKit::SilKit
)

set_target_properties(sil-kit-demo-can-echo-device 
    PROPERTIES
    #ensure SilKit shared libraries can be loaded
    INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN"
    BUILD_RPATH "$ORIGIN"
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTER_VCAN_OUTPUT_DIRECTORY}>
)
