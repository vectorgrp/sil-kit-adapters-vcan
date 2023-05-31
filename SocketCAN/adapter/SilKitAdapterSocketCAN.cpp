#include <cstdio>
#include <iostream>
#include <string.h>
#include <stdio.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <unistd.h>

#include <asio/posix/stream_descriptor.hpp>
#include "silkit/SilKit.hpp"
#include "silkit/config/all.hpp"
#include "silkit/services/can/all.hpp"
#include "silkit/services/can/string_utils.hpp"
#include "silkit/services/can/CanDatatypes.hpp"
#include <asio/ts/buffer.hpp>
#include <asio/ts/io_context.hpp>
#include <asio/ts/net.hpp>

#include "Exceptions.hpp"

using namespace std;
using namespace SilKit::Services::Can;

class CanConnection
{
public:
    CanConnection(asio::io_context& io_context, const std::string& canDevName,
                  std::function<void(can_frame)> onNewFrameHandler)
        : _canDeviceStream{io_context}
        , _onNewFrameHandler(std::move(onNewFrameHandler))
    {
        _canDeviceStream.assign(GetCanDeviceFileDescriptor(canDevName.c_str()));
        ReceiveCanFrameFromVirtualCanDevice();   
    }

    void SendCanFrameToCanDevice(const can_frame& frame)
    {

        auto sizeSent = _canDeviceStream.write_some(asio::buffer(&frame, sizeof(frame)));
        if (sizeof(frame) != sizeSent)
        {
            throw demo::InvalidFrameSizeError{};
        }
    }

private:
    int GetCanDeviceFileDescriptor(const char* canDeviceName)
    {
        struct ifreq ifr;
        int canFileDescriptor;
        int errorCode;

        if ((canFileDescriptor = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) // no can is open in this directory
        {
            return canFileDescriptor;
        }

        memset(&ifr, 0, sizeof(ifr));

        if (*canDeviceName)
        {
            strncpy(ifr.ifr_name, canDeviceName, IFNAMSIZ); // default is "can0"
        }
       
        errorCode = ioctl(canFileDescriptor, SIOCGIFINDEX, reinterpret_cast<void*>(&ifr));

        if (errorCode < 0)
        {
            close(canFileDescriptor);
            return errorCode;
        }

        struct sockaddr_can socketAddress;
        socketAddress.can_family = AF_CAN;
        socketAddress.can_ifindex = ifr.ifr_ifindex;

         // Bind socket (link socketCAN File Descriptor to address)
        /* Give the socket FD the local address ADDR (which is LEN bytes long).  */
        if (bind(canFileDescriptor, (struct sockaddr*)&socketAddress, sizeof(socketAddress)) < 0)
        {
            std::cerr << "Error in socket binding" << std::endl;
            return -2;
        }
        
        std::cout << "vCAN device successfully opened" << std::endl;
        return canFileDescriptor;
    }

    private:
    void ReceiveCanFrameFromVirtualCanDevice()
    {
        _canDeviceStream.async_read_some( asio::buffer(&_canFrameBuffer, sizeof(_canFrameBuffer)),
            [this](const std::error_code ec, const std::size_t bytes_received)
            {
                if (ec)
                {
                    throw demo::IncompleteReadError{};
                }
                auto frame_data = std::vector<std::uint8_t>(bytes_received);
                can_frame CF;
                asio::buffer_copy(asio::buffer(&CF, sizeof(CF)), asio::buffer(&_canFrameBuffer, sizeof(_canFrameBuffer)), bytes_received);
                _onNewFrameHandler(std::move(CF));
                ReceiveCanFrameFromVirtualCanDevice();
            });
    }
    
private:
    asio::posix::basic_stream_descriptor<> _canDeviceStream;
    struct can_frame _canFrameBuffer;
    std::function<void(can_frame)> _onNewFrameHandler;
};

inline can_frame SILKitToSocketCAN(const CanFrame& silkit_can_frame)
{
    struct can_frame socketcan_frame;
    socketcan_frame.can_id = silkit_can_frame.canId;
    socketcan_frame.can_dlc = silkit_can_frame.dlc;
    memset(&socketcan_frame.__pad, 0, sizeof(socketcan_frame.__pad)); // set padding to zero
    memset(&socketcan_frame.__res0, 0, sizeof(socketcan_frame.__res0)); // set reserved field to zero
    //socketcan_frame.len8_dlc = silkit_can_frame.dlc; // optional DLC for 8 byte payload length (9 .. 15)
    memcpy(socketcan_frame.data, silkit_can_frame.dataField.data(), silkit_can_frame.dataField.size());
    return socketcan_frame;
}

inline CanFrame SocketCANToSILKit(const struct can_frame& socketcan_frame)
{
    CanFrame silkit_frame;
    silkit_frame.canId = socketcan_frame.can_id;
    silkit_frame.flags = static_cast<CanFrameFlagMask>(socketcan_frame.can_id & CAN_EFF_FLAG); // get EFF/RTR/ERR flags
    silkit_frame.dlc = socketcan_frame.can_dlc;
    silkit_frame.sdt = 0; // not used in SocketCAN
    silkit_frame.vcid = 0; // not used in SocketCAN
    silkit_frame.af = 0; // not used in SocketCAN
    silkit_frame.dataField = SilKit::Util::Span<const uint8_t>(socketcan_frame.data, socketcan_frame.can_dlc);
    return silkit_frame;
}

int main(int argc, char** argv)
{
    const auto getArgDefault = [ argc, argv ](const std::string& argument, const std::string& defaultValue) -> auto
    {
        return [argc, argv, argument, defaultValue]() -> std::string {
            auto found = std::find_if(argv, argv + argc, [argument](const char* arg) -> bool {
                return arg == argument;
            });

            if (found != argv + argc && found + 1 != argv + argc)
            {
                return *(found + 1);
            }
            return defaultValue;
        };
    };

    const std::string canDevName = getArgDefault("--can-name", "can0")();
    const std::string registryURI = getArgDefault("--registry-uri", "silkit://localhost:8501")();
    const std::string participantName = getArgDefault("--participant-name", "SocketCAN_silkit")();
    const std::string canNetworkName = getArgDefault("--network-name", "CAN1")();
    const std::string canControllerName = participantName + "_CAN_CTRL";
    const std::string logLvl = getArgDefault("--log", "Info")();

    const std::string participantConfigurationString = R"({ "Logging": { "Sinks": [ { "Type": "Stdout", "Level": "Info" } ] } })";

    asio::io_context io_context;

    try
    {
        auto participantConfiguration = SilKit::Config::ParticipantConfigurationFromString(participantConfigurationString);
        std::cout << "Creating participant '" << participantName << "' at " << registryURI << std::endl;
        auto participant = SilKit::CreateParticipant(participantConfiguration, participantName, registryURI);

        std::cout << "Creating CAN controller '" << canControllerName << "'" << std::endl;
        auto* canController = participant->CreateCanController(canControllerName, canNetworkName);

        const auto onReceiveCanFrameFromCanDevice = [logLvl, canController](can_frame data) 
        {
            static intptr_t transmitId = 0;
            canController->SendFrame(SocketCANToSILKit(data), reinterpret_cast<void*>(++transmitId));

            if (logLvl.compare("Debug") == 0 || logLvl.compare("Trace") == 0)
            {
                std::cout << "CAN device >> SIL Kit: CAN frame (dlc=" << (int)data.can_dlc << " bytes, txId=" << transmitId << ")" << std::endl;
            }
        };

        std::cout << "Creating CAN device connector for '" << canDevName << "'" << std::endl;
        CanConnection canConnection{io_context, canDevName, onReceiveCanFrameFromCanDevice};

        const auto onReceiveCanMessageFromSilKit = [logLvl, &canConnection](ICanController* /*controller*/, const CanFrameEvent& msg) {
            CanFrame recievedFrame = msg.frame;
            canConnection.SendCanFrameToCanDevice(SILKitToSocketCAN(recievedFrame));

            if (logLvl.compare("Debug") == 0 || logLvl.compare("Trace") == 0)
            {
                std::cout << "SIL Kit >> CAN device: CAN frame (" << recievedFrame.dlc << " bytes)" << std::endl;
            }
        };

        const auto onCanAckCallback = [logLvl](ICanController* /*controller*/, const CanFrameTransmitEvent& ack) {
                if (logLvl.compare("Debug") == 0 || logLvl.compare("Trace") == 0)
                {
                    if (ack.status == CanTransmitStatus::Transmitted)
                    {
                        std::cout << "SIL Kit >> CAN : ACK for CAN Message with transmitId="
                            << reinterpret_cast<intptr_t>(ack.userContext) << std::endl;
                    }
                    else
                    {
                        std::cout << "SIL Kit >> CAN : NACK for CAN Message with transmitId="
                            << reinterpret_cast<intptr_t>(ack.userContext) << ": " << ack.status << std::endl;
                    }
                }
        };

        canController->AddFrameHandler(onReceiveCanMessageFromSilKit);
        canController->AddFrameTransmitHandler(onCanAckCallback);
        canController->Start();

        io_context.run();

        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();

    }
    catch (const SilKit::ConfigurationError& error)
    {
        std::cerr << "Invalid configuration: " << error.what() << std::endl;
        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();
        return -2;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Something went wrong: " << error.what() << std::endl;
        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();
        return -3;
    }

    return 0;
}