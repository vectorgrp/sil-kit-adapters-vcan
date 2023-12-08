add_executable(SilKitAdapterSocketCAN
    "SilKitAdapterSocketCAN.cpp"
    "Parsing.cpp")
target_link_libraries(SilKitAdapterSocketCAN
    PRIVATE
    Utility
	asio
	SilKit::SilKit
)
set_target_properties(SilKitAdapterSocketCAN 
	PROPERTIES
	#ensure SilKit shared libraries can be loaded
    INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN"
    BUILD_RPATH "$ORIGIN"
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTERS_VCAN_OUTPUT_DIRECTORY}>
)
add_custom_command(
    TARGET SilKitAdapterSocketCAN
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy $<TARGET_FILE:SilKit::SilKit> ${SILKIT_ADAPTERS_VCAN_LIBRARY_DIRECTORY}/libSilKit.so
)

