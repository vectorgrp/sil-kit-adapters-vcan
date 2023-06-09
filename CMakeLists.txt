cmake_minimum_required(VERSION 3.5)
project(VectorSilKitAdaptersVCan)
include(FetchContent)

set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)

set(SILKIT_ADAPTERS_VCAN_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

set(SILKIT_VERSION "4.0.28" CACHE STRING "If no SIL Kit package is specified, this version will be downloaded")
set(SILKIT_FLAVOR "ubuntu-18.04-x86_64-gcc" CACHE STRING "If no SIL Kit package is specified, this package flavor will be downloaded")

find_package(Threads REQUIRED)
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/asio/include)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE=1)
target_link_libraries(asio INTERFACE Threads::Threads)

if(DEFINED SILKIT_PACKAGE_DIR)    
# if the user forces using a specific SIL Kit package, use it
	message(STATUS "SILKIT_PACKAGE_DIR has been set to: ${SILKIT_PACKAGE_DIR}, CMAKE will look for a SIL Kit package in that directory")
    find_package(SilKit 4.0.7
        REQUIRED
        CONFIG
        NO_CMAKE_PACKAGE_REGISTRY
        NO_DEFAULT_PATH
        PATHS "${SILKIT_PACKAGE_DIR}"
    )
	
else()
# otherwise, look for an installed version of SIL Kit
    message(STATUS "SILKIT_PACKAGE_DIR has not been set by user. Attempting to find an installed version of SIL Kit")

    find_package(SilKit 4.0.7
        CONFIG
    )

    if(NOT SilKit_FOUND)
		# if installed SIL Kit is not found, fetch it from github.com
		message(STATUS "No installed version of SIL Kit was found. Attempting to fetch [SilKit-${SILKIT_VERSION}-${SILKIT_FLAVOR}] from github.com")
        FetchContent_Declare(
                silkit
                URL https://github.com/vectorgrp/sil-kit/releases/download/sil-kit%2Fv${SILKIT_VERSION}/SilKit-${SILKIT_VERSION}-${SILKIT_FLAVOR}.zip
                DOWNLOAD_DIR ${CMAKE_CURRENT_LIST_DIR}/Downloads
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            )

        message(STATUS "SIL Kit: fetching [SilKit-${SILKIT_VERSION}-${SILKIT_FLAVOR}]")
        FetchContent_MakeAvailable(silkit)

        set(SILKIT_SOURCE_DIR ${silkit_SOURCE_DIR}/SilKit-Source)
        message(STATUS "SIL Kit: using source code from: \"${SILKIT_SOURCE_DIR}\"")
        message(STATUS "SIL Kit: using pre-built binaries from: ${silkit_SOURCE_DIR}")

        message(STATUS "Searching SilKit package which has been fetched from github.com")
        find_package(SilKit 4.0.7
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

add_subdirectory(SocketCAN/Utility)
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
    DESTINATION .
    COMPONENT source
    REGEX "\.git$" EXCLUDE
    REGEX "\.github$" EXCLUDE    
)

install(
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    DESTINATION .
    COMPONENT source
    REGEX "\.git$" EXCLUDE
    REGEX "\.github$" EXCLUDE
)

install(
    FILES
        CMakeLists.txt
        CMakePresets.json
    DESTINATION .
    COMPONENT source
)

install(
    FILES
        LICENSE
        README.md        
    DESTINATION .
    COMPONENT source
)

install(
    DIRECTORY
        ${SILKIT_ADAPTERS_VCAN_OUTPUT_DIRECTORY}
    DESTINATION build
    COMPONENT bin
)

###############################################################################
# Packaging
###############################################################################

set(CPACK_GENERATOR "ZIP")

set(CPACK_PACKAGE_DESCRIPTION "binary release of SIL Kit Adapters vCAN")
set(CPACK_PACKAGE_NAME "SilKit-Adapters-vCAN")
set(CPACK_PACKAGE_VENDOR "Vector Informatik")
set(CPACK_PACKAGE_CONTACT "support@vector.com")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}${PACKAGE_FILENAME_SUFFIX}")

include(CPack)
