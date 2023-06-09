﻿# SocketCAN Demo and Adapter Setup
This demo consists of SocketCAN interface `can0` which is connected to SIL Kit via `SilKitAdapterSocketCAN`, with the latter being a SIL Kit participant connected to `CAN1` SIL Kit network. 
For demonstration purposes, another SIL Kit participant `CanEchoDevice` is connected to the same `CAN1` SIL Kit network.   

First, a virtual CAN interface (SocketCAN) is set up in Linux using a shell script. Then, the `SilKitAdapterSocketCAN` is launched, binding to the virtual CAN interface from one side and connecting to SIL Kit as a participant from the other. 
After that the `CanEchoDevice` is launched and its connection to SIL Kit is established. 
When all the connections are set up, a shell script is run to generate CAN payload on `can0` SocketCAN interface. 

The following sketch shows the general setup: 

    +------------[ Virtual CAN ]--------------+                      +-----[ SIL Kit Adapter SocketCAN ]-----+
    |-CAN payload generation on [can0]        <----- SocketCAN ------>    connecting SocketCAN [can0] to     |
    |-Sending & receiving on [can0] interface |                      |          SIL Kit [CAN1] Network       |
    +-----------------------------------------+                      +------ʌ--------------------------------+
                                                                            |            
                                                                            |           
                                                                     +------|------[ SIL Kit ]---------------+
                                                                     |      |                                |
                                                                     |  +---v---[ CAN1 Network ]----------+  |
    +--------------[ CanEchoDevice ]----------+                      |  |                                 |  |
    |                                         <----------------------|-->     Peer-to-peer connection     |  |
    +-----------------------------------------+                      |  |     between participants on     |  |
    +--------------[ Vector CANoe ]-----------+                      |  |     SIL Kit [CAN1] Network      |  |
    |                                         <----------------------|-->                                 |  |
    +-----------------------------------------+                      |  +---------------------------------+  |

## CanEchoDevice

This demo application implements a very simple SIL Kit participant that connects to the SIL Kit `CAN1` network, to which both the SocketCAN Adapter and CANoe can connect as SIL Kit 
participants in order to exchange CAN frames. The `CanEchoDevice` responds to CAN messages it receives on `CAN1` network by incrementing the received CAN ID by `1` and shifitng the data field by one byte to the left, then sending it back on the same `CAN1` network.

# Running the Demos

## SocketCAN interface setup and CAN traffic generation
First, run the `OpenVCan_can0.sh` script in the Terminal, which will load the necessary Linux kernel modules and setup a virtual CAN interface named `can0`.

    sudo chmod +x SocketCAN/demos/shell_scripts/OpenVCan_can0.sh
    ./SocketCAN/demos/shell_scripts/OpenVCan_can0.sh

You should see this output that indicates that the `can0` virtual CAN device has been successfully launched:
    
    Creating Virtual CAN interface with name [can0]
    Verifying can0 interface state:
    vCAN interface [can0] is up & running.

Now, you can start sending virtual CAN frames from your terminal. 
This can be done with the following command
    
    cansend can0 123#AAAABBBB

**Note:** In this example: `can0` is the name of the virtual CAN interface, `123` is the CAN frame ID, `AAAABBBB` is the CAN transmitted data

Run the SendSocketCANFrames.sh script to continuously generate a `001#AAAABBBB` CAN message (CAN ID = 001, Data=AAAABBBB) and send it through to `can0`.

    sudo chmod +x SocketCAN/demos/shell_scripts/SendSocketCANFrames.sh
    ./SocketCAN/demos/shell_scripts/SendSocketCANFrames.sh

As of now, a virtual CAN `can0` device is up and running and a `001#AAAABBBB` message is continuously being sent on it.

## Running the Demo Applications

Now is a good point to start the `sil-kit-registry`:

    ./path/to/SilKit-x.y.z-$ubuntu/SilKit/bin/sil-kit-registry --listen-uri 'silkit://0.0.0.0:8501'

After that, launch the SilKitAdapterSocketCAN

    ./build/bin/SilKitAdapterSocketCAN

You should see the following output in the terminal where the adapter was launched: 

    Creating participant 'SocketCAN_silkit' at silkit://localhost:8501
    [date time] [SocketCAN_silkit] [info] Creating participant 'SocketCAN_silkit' at 'silkit://localhost:8501', SIL Kit version: 4.0.26
    [date time] [SocketCAN_silkit] [info] Connected to registry at 'tcp://127.0.0.1:8501' via 'tcp://127.0.0.1:53722' (silkit://localhost:8501)
    Creating CAN controller 'SocketCAN_silkit_CAN_CTRL'
    Creating CAN device connector for 'can0'
    vCAN device successfully opened

You should see also a `SocketCAN_silkit` participant announcement in the SIL Kit registry terminal

    [date time] [SilKitRegistry] [info] Sending known participant message to SocketCAN_silkit

In a separate Terminal, launch the CanEchoDevice

    ./build/bin/SilKitDemoCanEchoDevice

You should see the following output in the terminal after launching the CanEchoDevice:

    Creating participant 'CanEchoDevice' at silkit://localhost:8501
    [date time] [CanEchoDevice] [info] Creating participant 'CanEchoDevice' at 'silkit://localhost:8501', SIL Kit version: 4.0.26
    [date time] [CanEchoDevice] [info] Connected to registry at 'tcp://127.0.0.1:8501' via 'tcp://127.0.0.1:57750' (silkit://localhost:8501)
    Creating CAN controller 'CanEchoDevice_CAN1'
    Press enter to stop the process...
    SIL Kit >> Demo: CAN frame (4 bytes)
    SIL Kit >> Demo : ACK for CAN Message with transmitId=1
    Demo >> SIL Kit : CAN frame (dlc=4 bytes, txId=1)
    SIL Kit >> Demo: CAN frame (4 bytes)
    SIL Kit >> Demo : ACK for CAN Message with transmitId=2
    Demo >> SIL Kit : CAN frame (dlc=4 bytes, txId=2)
    SIL Kit >> Demo: CAN frame (4 bytes)
    SIL Kit >> Demo : ACK for CAN Message with transmitId=3
    Demo >> SIL Kit : CAN frame (dlc=4 bytes, txId=3)
    . 
    . 

You should also see a `CanEchoDevice` participant announcement in the SIL Kit registry terminal:

    [date time] [SilKitRegistry] [info] Sending known participant message to CanEchoDevice


### Monitoring data on `can0` 
You can read data that is available on the `can0` SocketCAN device (both outgoing and incoming), to do this you can use the following command in a separate Terminal:

    candump can0

If both the `SendSocketCANFrames.sh` script and the CanEchoDevice are running along with the SilKitAdapterSocketCAN, you should see output similar to the following in the terminal:
    
    can0  001   [4]  AA AA BB BB
    can0  002   [4]  AA BB BB 00
    can0  001   [4]  AA AA BB BB
    can0  002   [4]  AA BB BB 00
    . 
    .

As described earlier, CAN messages with ID of 001 are the ones sent from can0 (outgoing). On the other hand, the ones with ID 002 are being received on `can0`, these have been sent back from the CanEchoDevice after increasing ID by `1` and applying a shift-left of data by one byte. 


## Adding CANoe (16 SP3 or newer) as a participant
If CANoe is connected to the SIL Kit, all CAN traffic is visible there as well. You can also execute a test unit which checks if the CAN messages are being transmitted as expected.

Before you can connect CANoe to the SIL Kit network you should adapt the `RegistryUri` in `/SocketCAN/demos/CANoe_SILKit_config.silkit.yaml` to the IP address of your system where your sil-kit-registry is running. 
The configuration file is referenced by both following CANoe use cases (Desktop Edition and Server Edition).

### CANoe Desktop Edition
Load the `Configuration_vcan.cfg` from the `demos/CANoe` directory and start the measurement. Optionally you can also start the test unit execution of included test configuration.
While the demo is running these tests should be successful.

### CANoe4SW Server Edition (Windows)
You can also run the same test set with `CANoe4SW SE` by executing the following powershell script `demos/CANoe4SW_SE/run.ps1`. 
The test cases are executed automatically and you should see a short test report in powershell after execution.

### CANoe4SW Server Edition (Linux)
You can also run the same test set with `CANoe4SW SE (Linux)`. At first you have to execute the powershell script `demos/CANoe4SW_SE/createEnvForLinux.ps1` on your windows system by using tools of `CANoe4SW SE (Windows)` to prepare your test environment for Linux. 
In `demos/CANoe4SW_SE/run.sh` you should adapt `canoe4sw_se_install_dir` to the path of your `CANoe4SW SE` installation in your WSL2. Afterwards you can execute `demos/CANoe4SW_SE/run.sh` in your WSL2. The test cases are executed automatically and you should see a short test report in your terminal after execution.

