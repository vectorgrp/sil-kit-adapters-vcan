# SocketCAN Demo and Adapter Setup
This demo consists of SocketCAN interface connection with SIL Kit. The SocketCAN interface "can0" is connected to the SIL Kit via ``SilKitAdapterSocketCAN``, with the latter being a SIL Kit participant.  
First, a virtual CAN interface (SocketCAN) is set up in Linux using a shell script. Then, the SilKitAdapterSocketCAN executable is launched, binding to the virtual CAN interface from one side and connecting to SIL Kit as a participant from the other. 
When the connections are set up, a shell script in launched to generate CAN payload. 

The following sketch shows the general setup: 

    +-----[   Virtual CAN (Linux)  ]-----------+                                               +------[ SIL Kit Adapter SocketCAN ]------+
    |  -CAN payload generation                 | => ----------- SocketCAN  --------------- =>  |     CanConnection to SocketCAN          |
    |  -Sending CAN payload on can0 interface  |                                               |     virtual (SIL Kit) CAN               |
    +------------------------------------------+                                               +-----------------------------------------+
                                                                                                                   <=>
                                                                                                                 SIL Kit
                                                                                                                   <=>                 
                                                                                               +-----------[ SIL Kit Registry ]----------+
                                                                                               |                                         |
                                                                                               |                                         |
                                                                                               |                                         |
    +--------------[ Vector CANoe ]--------------+                                             |                                         |
    |                                            | <= ------------- SIL Kit --------------- => |                                         |
    +--------------------------------------------+                                             +-----------------------------------------+
  

# Running the Demos

## Running the Demo Applications
First, run the ``OpenVCan_can.sh`` script, which will load the necessary Linux kernel modules and setup a virtual CAN interface named ``can0``.

    sudo chmod +x /SocketCAN/demos/shell_scripts/OpenVCan_can0.sh
    ./SocketCAN/demos/shell_scripts/OpenVCan_can0.sh
    
Then, start the ``sil-kit-registry``:

    ./path/to/SilKit-x.y.z-$platform/SilKit/bin/sil-kit-registry --listen-uri 'silkit://127.0.0.1:8501'

After that, launch the SilKitAdapterSocketCAN

    ./build/bin/SilKitAdapterSocketCAN

You should see the following output in the terminal after launching the adapter: 

    Creating participant 'SocketCAN_silkit' at silkit://localhost:8501
    [date time] [SocketCAN_silkit] [info] Creating participant 'SocketCAN_silkit' at 'silkit://localhost:8501', SIL Kit version: 4.0.17
    [date time] [SocketCAN_silkit] [info] Connected to registry at 'tcp://127.0.0.1:8501' via 'tcp://127.0.0.1:57780' (silkit://localhost:8501)
    Creating CAN controller 'SocketCAN_silkit_CAN_CTRL'
    Creating CAN device connector for 'can0'
    vCAN device successfully opened

Also, you should see the ``SocketCAN_silkit`` participant announcement in the SIL Kit registry terminal

    [SilKitRegistry] [info] Sending known participant message to SocketCAN_silkit


## Starting CANoe
You can also start ``CANoe 16 SP3`` or newer and load the ``Configuration_vcan.cfg`` from the ``/SocketCAN/demos/CANoe`` directory and start the
measurement.

## CAN traffic generation
Now, you can start sending virtual CAN frames from your terminal. 
This can be done with the following command
    
    cansend can0 123#AAAABBBB

**Note:** In this example: ``can0`` is the name of the virtual CAN interface, ``123`` is the CAN frame ID, ``AAAABBBB`` is the CAN transmitted data

Instead of manually generating CAN messages from terminal, you can run the SendCANFrames.sh script to automatically generate some CAN messages and send them trough to ``can0``.

    sudo chmod +x /SocketCAN/demos/shell_scripts/SendSocketCANFrames.sh
    ./SocketCAN/demos/shell_scripts/SendSocketCANFrames.sh

If CANoe is connected to the SIL Kit, all CAN traffic should be visible there as well.
