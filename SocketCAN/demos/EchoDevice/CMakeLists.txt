add_executable(SilKitDemoCanEchoDevice
    "SilKitDemoCanEchoDevice.cpp"
)
target_link_libraries(SilKitDemoCanEchoDevice
    PRIVATE
	Utility
	SilKit::SilKit
)
set_target_properties(SilKitDemoCanEchoDevice 
	PROPERTIES
	#ensure SilKit shared libraries can be loaded
    INSTALL_RPATH "$ORIGIN"
    BUILD_RPATH "$ORIGIN"
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTERS_VCAN_OUTPUT_DIRECTORY}>
)
