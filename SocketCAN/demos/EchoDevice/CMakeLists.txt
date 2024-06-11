add_executable(SilKitDemoCanEchoDevice
    "SilKitDemoCanEchoDevice.cpp"
    ${CMAKE_SOURCE_DIR}/SocketCAN/adapter/Parsing.cpp
    ${CMAKE_SOURCE_DIR}/SocketCAN/adapter/SignalHandler.cpp
)
target_include_directories(SilKitDemoCanEchoDevice 
    PRIVATE
    "${CMAKE_SOURCE_DIR}/SocketCAN/Utility/"
    "${CMAKE_SOURCE_DIR}/SocketCAN/"
)
target_link_libraries(SilKitDemoCanEchoDevice
    PRIVATE
    asio
    SilKit::SilKit
)
set_target_properties(SilKitDemoCanEchoDevice 
    PROPERTIES
    #ensure SilKit shared libraries can be loaded
    INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN"
    BUILD_RPATH "$ORIGIN"
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTERS_VCAN_OUTPUT_DIRECTORY}>
)
