# Vector SIL Kit Adapter for virtual CAN (Linux only)
This collection of software is provided to illustrate how the [Vector SIL Kit](https://github.com/vectorgrp/sil-kit/)
can be attached to a virtual CAN (Controller Area Network) interface [SocketCAN](https://docs.kernel.org/networking/can.html) running in Linux kernel.

This repository contains instructions to build the adapter and set up a minimal working development environment.

The main contents are working examples of necessary software to connect the running system to a SIL Kit environment,
as well as complimentary demo applications for some communication to happen.

Those instructions assume you use a Linux OS (or a virtual machine running a Linux image) for building and running the adapter and use ``bash`` as your interactive
shell. 

**Note:** WSL/WSL2 are excluded from this context as their standard kernels don't support SocketCAN.   

## a) Getting Started with self-built Adapter and Demos

This section specifies steps you should do if you have just cloned the repository.

First, change your current directory to the top-level in the ``sil-kit-adapters-vcan``
repository:

    cd /path/to/sil-kit-adapters-vcan

### Fetch Third Party Software
The first thing that you should do is initializing the submodules to fetch the required third party software:

    git submodule update --init --recursive

Otherwise clone the standalone version of asio manually:

    git clone --branch asio-1-24-0 https://github.com/chriskohlhoff/asio.git third_party/asio

### Build the Adapter
To build the adapter, you'll need an ubuntu SIL Kit package ``SilKit-x.y.z-$ubuntu``. You can download it directly from [Vector SIL Kit Releases](https://github.com/vectorgrp/sil-kit/releases). The easiest way would be to download it with your web browser, unzip it and place it on your file system, where it also can be accessed by ``bash``.

The adapter and demos are built using ``cmake``:

    mkdir build
    cmake -S. -Bbuild -DSILKIT_PACKAGE_DIR=/path/to/SilKit-x.y.z-$ubuntu/ -D CMAKE_BUILD_TYPE=Release
    cmake --build build --parallel

**Note 1:** If you have a self-built or pre-built version of SIL Kit, you can build the adapter against it by setting SILKIT_PACKAGE_DIR to the path, where the bin, include and lib directories are.

**Note 2:** If you have SIL Kit installed on your system, you can build the adapter against it, even by not providing SILKIT_PACKAGE_DIR to the installation path at all.

**Note 3:** If you don't provide a specific path for SILKIT_PACKAGE_DIR and there is no SIL Kit installation on your system, a SIL Kit release package (the default version listed in CMakeLists.txt) will be fetched from github.com and the adapter will be built against it.

The adapter and demo executables will be available in the ``bin`` directory.
Additionally the ``SilKit`` shared library is copied to the ``lib`` directory next to it automatically.

### Build the adapter for Android environments 
You can use the [Android NDK](https://developer.android.com/ndk) to cross-build the adapter for Android environments. 

Begin by cloning the SIL Kit repo and building SIL Kit using the cmake toolchain file provided by Android NDK and generating a package as follows: 

    cmake -S. -Bbuild -DCMAKE_TOOLCHAIN_FILE=/path/to/android-ndk/build/cmake/android.toolchain.cmake -DANDROID_ABI=x86_64 -DSILKIT_HOST_PLATFORM=android -DANDROID_PLATFORM=android-33 -DSILKIT_BUILD_TESTS=OFF -DSILKIT_BUILD_DEMOS=OFF

    cmake --build build --parallel --target package

Unzip the package that was generated and use the same toolchain file to cross-build the adapter, ensuring it is built against the unzipped package. 

    cmake -S.  -Bbuild -DCMAKE_TOOLCHAIN_FILE=/path/to/android-ndk/build/cmake/android.toolchain.cmake  -DANDROID_ABI=x86_64 -DCMAKE_BUILD_TYPE=Release -DSILKIT_HOST_PLATFORM=android  -DSILKIT_PACKAGE_DIR=/path/to/SilKit-*-Linux-x86_64-clang-Release/  -DSilKit_DIR=/path/to/SilKit-*-Linux-x86_64-clang-Release/lib/cmake/SilKit -DANDROID_PLATFORM=android-33

    cmake --build build --parallel


Lastly, update the LD_LIBRARY_PATH in your Android environment to point to the location of the SIL Kit shared library, which can be found in the generated lib folder.

## b) Getting Started with pre-built Adapter and Demos
Download a preview or a release of the adapter directly from [Vector SIL Kit Adapter for virtual CAN Releases](https://github.com/vectorgrp/sil-kit-adapters-vcan/releases).

You should also download a SIL Kit Release directly from [Vector SIL Kit Releases](https://github.com/vectorgrp/sil-kit/releases). You will need this for being able to start a sil-kit-registry.

## Install the sil-kit-adapter-vcan (optional)
If you call the following command (can be done for self-built and pre-built package after cmake configure) ``sil-kit-adapter-vcan`` can be called from everywhere without defining a path:  

    sudo cmake --build build --target install

The default installation path will be ``/usr/local/bin``. Be aware that SIL Kit itself also needs to be installed to make this work.

## Run the sil-kit-adapter-vcan
This application allows the user to attach virtual CAN interfaces (``SocketCAN``) running in Linux environment to the
SIL Kit.

Before you start the adapter there always needs to be a sil-kit-registry running already. Start it e.g. like this:

    ./path/to/SilKit-x.y.z-$ubuntu/SilKit/bin/sil-kit-registry --listen-uri 'silkit://0.0.0.0:8501'

It is also necessary that a virtual CAN interface is already opened and running in Linux kernel before you run the adapter. This can be done using the following terminal commands: 

    sudo modprobe vcan
    ip link add dev can0 type vcan
    ip link set up can0

**Note:** These commands will open a virtual CAN interface with the name ``can0``.

Now you can run the adapter from terminal. 

The application *optionally* takes the following command line arguments:

    sil-kit-adapter-vcan [--name <participant name{SilKitAdapterVcan}>]
        [--configuration <path to .silkit.yaml or .json configuration file>]
        [--registry-uri silkit://<host{localhost}>:<port{8501}>]
        [--log <Trace|Debug|Warn|{Info}|Error|Critical|Off>]
        [--can-name <vcan device name{can0}>]
        [--network <SIL Kit CAN network{CAN1}>]
        [--help]
**Note:** SIL Kit-specific CLI arguments will be overwritten by the config file specified by ``--configuration``.

## SocketCAN Demo
The aim of this demo is to showcase a simple adapter that forwards CAN traffic from Linux terminal running a virtual CAN interface through to
Vector SIL Kit. 

This demo is further explained in [SocketCAN/README.md](SocketCAN/README.md).

## QEMU Demo
This demo is a step-by-step guide on how to use the adapter to link a QEMU image with an exposed SocketCAN interface to a SIL Kit CAN network, as well as some instructions to setup the QEMU image with an exposed SocketCAN interface.  

This demo is further explained in [QEMU/README.md](QEMU/README.md).
