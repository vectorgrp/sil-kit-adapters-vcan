cmake_minimum_required(VERSION 3.5)
project(VectorSilKitAdapterVcan)
include(FetchContent)

set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)

set(SILKIT_ADAPTER_VCAN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(SILKIT_ADAPTER_VCAN_LIBRARY_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

set(SILKIT_VERSION "4.0.50" CACHE STRING "If no SIL Kit package is specified, this version will be downloaded")
set(SILKIT_FLAVOR "ubuntu-18.04-x86_64-gcc" CACHE STRING "If no SIL Kit package is specified, this package flavor will be downloaded")

find_package(Threads REQUIRED)
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/asio/include)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE=1)
target_link_libraries(asio INTERFACE Threads::Threads)

if(DEFINED SILKIT_PACKAGE_DIR)    
# if the user forces using a specific SIL Kit package, use it
    message(STATUS "SILKIT_PACKAGE_DIR has been set to: ${SILKIT_PACKAGE_DIR}, CMAKE will look for a SIL Kit package in that directory")
    find_package(SilKit 4.0.40
        REQUIRED
        CONFIG
        NO_CMAKE_PACKAGE_REGISTRY
        NO_DEFAULT_PATH
        PATHS "${SILKIT_PACKAGE_DIR}"
    )
	
else()
# otherwise, look for an installed version of SIL Kit
    message(STATUS "SILKIT_PACKAGE_DIR has not been set by user. Attempting to find an installed version of SIL Kit")
    
    find_package(SilKit 4.0.40
        CONFIG
    )
    
    if(NOT SilKit_FOUND)
        # if installed SIL Kit is not found, fetch it from github.com
        message(STATUS "No installed version of SIL Kit was found. Attempting to fetch [SilKit-${SILKIT_VERSION}-${SILKIT_FLAVOR}] from github.com")
        FetchContent_Declare(
            silkit
            URL https://github.com/vectorgrp/sil-kit/releases/download/sil-kit%2Fv${SILKIT_VERSION}/SilKit-${SILKIT_VERSION}-${SILKIT_FLAVOR}.zip
            DOWNLOAD_DIR ${CMAKE_CURRENT_LIST_DIR}/Downloads                
        )

        message(STATUS "SIL Kit: fetching [SilKit-${SILKIT_VERSION}-${SILKIT_FLAVOR}]")
        FetchContent_MakeAvailable(silkit)

        set(SILKIT_SOURCE_DIR ${silkit_SOURCE_DIR}/SilKit-Source)
        message(STATUS "SIL Kit: using source code from: \"${SILKIT_SOURCE_DIR}\"")
        message(STATUS "SIL Kit: using pre-built binaries from: ${silkit_SOURCE_DIR}")

        message(STATUS "Searching SilKit package which has been fetched from github.com")
        find_package(SilKit 4.0.40
            REQUIRED
            CONFIG
            NO_CMAKE_PACKAGE_REGISTRY
            NO_DEFAULT_PATH
            PATHS "${silkit_SOURCE_DIR}"
        )
    endif()
endif()

if(TARGET SilKit::SilKit)
    message(STATUS "SIL Kit package has been successfully imported as a CMake target. [version : ${SilKit_VERSION}]")
else()
    message(FATAL_ERROR "Something went wrong : Could not find SIL Kit package.")
endif()

add_subdirectory(SocketCAN/adapter)
add_subdirectory(SocketCAN/demos)

################################################################################
# Distribution of the source code and binaries
################################################################################
# Install sources and binaries
# Copy all files from the source and bin directory to the proper destination
# Leave out git repo related data

install(
    DIRECTORY            
        ${CMAKE_CURRENT_SOURCE_DIR}/SocketCAN
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party        
    DESTINATION .
    COMPONENT source
    EXCLUDE_FROM_ALL   
    REGEX "\.git$" EXCLUDE
    REGEX "\.github$" EXCLUDE    
)

install(
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/SocketCAN/demos/shell_scripts/setup_vcan_device.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/SocketCAN/demos/shell_scripts/send_CAN_frames.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/SocketCAN/demos/shell_scripts/setup_vcan_start_adapter_send_CAN_frames.sh
    DESTINATION SocketCAN/demos/shell_scripts
    PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
    COMPONENT source
    EXCLUDE_FROM_ALL
)

install(
    FILES            
        ${CMAKE_CURRENT_SOURCE_DIR}/SocketCAN/demos/CANoe4SW_SE/run.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/SocketCAN/demos/CANoe4SW_SE/run_all.sh
    DESTINATION SocketCAN/demos/CANoe4SW_SE
    PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
    COMPONENT source
    EXCLUDE_FROM_ALL
)

install(
    FILES
        CMakeLists.txt
        CMakePresets.json
        LICENSE
        README.md
        SECURITY.md
        CONTRIBUTING.md
    DESTINATION .
    COMPONENT source
    EXCLUDE_FROM_ALL
)

include(GNUInstallDirs)

install(
    FILES 
        ${SILKIT_ADAPTER_VCAN_LIBRARY_DIRECTORY}/libSilKit.so                
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PERMISSIONS
        OWNER_READ OWNER_WRITE
        GROUP_READ
        WORLD_READ
    COMPONENT lib
    EXCLUDE_FROM_ALL
)

install(
    TARGETS 
        sil-kit-demo-can-echo-device
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime
    PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE    
    EXCLUDE_FROM_ALL
)

# only sil-kit-adapter-vcan should be installed to /usr/local/bin by calling --target install (therefore it is not excluded)
install(
    TARGETS 
        sil-kit-adapter-vcan
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Runtime
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/sil-kit-adapter-vcan COMPONENT Development   
    PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
)

###############################################################################
# Packaging
###############################################################################

set(CPACK_GENERATOR "ZIP")

set(CPACK_PACKAGE_DESCRIPTION "binary release of SIL Kit Adapter vcan")
set(CPACK_PACKAGE_NAME "SilKit-Adapter-vcan")
set(CPACK_PACKAGE_VENDOR "Vector Informatik")
set(CPACK_PACKAGE_CONTACT "support@vector.com")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}${PACKAGE_FILENAME_SUFFIX}")

set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE ON)
set(CPACK_COMPONENT_INCLUDE_TOPLEVEL_DIRECTORY ON)

include(CPack)
