add_executable(sil-kit-adapter-vcan
    "SilKitAdapterSocketCAN.cpp"
    "ConnectionsImplementations.cpp"
    "AdapterConnections.cpp"
)

target_link_libraries(sil-kit-adapter-vcan
    PRIVATE
    asio
    sil-kit-adapters-common
    SilKit::SilKit
)

set_target_properties(sil-kit-adapter-vcan
    PROPERTIES
    #ensure SilKit shared libraries can be loaded
    INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN"
    BUILD_RPATH "$ORIGIN"
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTER_VCAN_OUTPUT_DIRECTORY}>
)

copy_sil_kit_to_bin(sil-kit-adapter-vcan 
    ${SILKIT_ADAPTER_VCAN_OUTPUT_DIRECTORY} 
    ${SILKIT_ADAPTER_VCAN_LIBRARY_DIRECTORY}
)