
add_executable(SilKitDemoCanEchoDevice
    "SilKitDemoCanEchoDevice.cpp"
)
target_link_libraries(SilKitDemoCanEchoDevice
    PRIVATE
        Utility
        SilKit::SilKit
)
set_target_properties(SilKitDemoCanEchoDevice PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTERS_VCAN_OUTPUT_DIRECTORY}>
)
