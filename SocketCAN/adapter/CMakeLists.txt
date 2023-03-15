
add_executable(SilKitAdapterSocketCAN
    "SilKitAdapterSocketCAN.cpp"
)
target_link_libraries(SilKitAdapterSocketCAN
    PRIVATE
        Utility
        asio
        SilKit::SilKit
)
set_target_properties(SilKitAdapterSocketCAN PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTERS_VCAN_OUTPUT_DIRECTORY}>
)
add_custom_command(
    TARGET SilKitAdapterSocketCAN
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy $<TARGET_PROPERTY:SilKit::SilKit,LOCATION> ${SILKIT_ADAPTERS_VCAN_OUTPUT_DIRECTORY}
)

