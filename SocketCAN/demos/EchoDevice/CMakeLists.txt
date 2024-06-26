add_executable(sil-kit-demo-can-echo-device
    "SilKitDemoCanEchoDevice.cpp"
    "${CMAKE_SOURCE_DIR}/SocketCAN/adapter/Parsing.cpp"
    "${CMAKE_SOURCE_DIR}/SocketCAN/Utility/SignalHandler.cpp"
)
target_include_directories(sil-kit-demo-can-echo-device 
    PRIVATE
    "${CMAKE_SOURCE_DIR}/SocketCAN/Utility/"
    "${CMAKE_SOURCE_DIR}/SocketCAN/"
)
target_link_libraries(sil-kit-demo-can-echo-device
    PRIVATE
    asio
    SilKit::SilKit
)
set_target_properties(sil-kit-demo-can-echo-device 
    PROPERTIES
    #ensure SilKit shared libraries can be loaded
    INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN"
    BUILD_RPATH "$ORIGIN"
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTERS_VCAN_OUTPUT_DIRECTORY}>
)
