add_executable(sil-kit-adapter-vcan
    "SilKitAdapterSocketCAN.cpp"
    "Parsing.cpp"
    "../Utility/SignalHandler.cpp"
	"ConnectionsImplementations.cpp"
	"AdapterConnections.cpp"
)
target_include_directories(sil-kit-adapter-vcan
    PRIVATE
    "${CMAKE_SOURCE_DIR}/SocketCAN/Utility/"
)
target_link_libraries(sil-kit-adapter-vcan
    PRIVATE
    asio
    SilKit::SilKit
)
set_target_properties(sil-kit-adapter-vcan
    PROPERTIES
    #ensure SilKit shared libraries can be loaded
    INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN"
    BUILD_RPATH "$ORIGIN"
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTERS_VCAN_OUTPUT_DIRECTORY}>
)
add_custom_command(
    TARGET sil-kit-adapter-vcan
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy $<TARGET_FILE:SilKit::SilKit> ${SILKIT_ADAPTERS_VCAN_LIBRARY_DIRECTORY}/libSilKit.so
)

