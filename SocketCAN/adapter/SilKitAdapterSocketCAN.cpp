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
#include "Parsing.hpp"
#include "SignalHandler.hpp"

#include "SilKitAdapterSocketCAN.hpp"

using namespace std;
using namespace SilKit::Services::Can;
using namespace exceptions;
using namespace adapters;
using namespace SilKit::Services::Orchestration;

class CanConnection
{
public:
    CanConnection(asio::io_context& io_context, const std::string& canDevName,
                  std::function<void(can_frame)> onNewFrameHandler, SilKit::Services::Logging::ILogger* logger)
        : _canDeviceStream{io_context}
        , _onNewFrameHandler(std::move(onNewFrameHandler))
        , _logger(logger)
    {
        _fileDescriptor = GetCanDeviceFileDescriptor(canDevName.c_str());
        throwInvalidFileDescriptorIf(_fileDescriptor < 0);

        _canDeviceStream.assign(_fileDescriptor);
        ReceiveCanFrameFromVirtualCanDevice();
    }

    void SendCanFrameToCanDevice(const can_frame& frame)
    {
        auto sizeSent = _canDeviceStream.write_some(asio::buffer(&frame, sizeof(frame)));
        if (sizeof(frame) != sizeSent)
        {
            throw exceptions::InvalidFrameSizeError{};
        }
    }

private:
    
    std::string extractErrorMessage(int errorCode)
    {
        const char* errorMessage = strerror(errorCode);

        if (errorMessage != nullptr)
        {
            return ("\t(" + std::string(errorMessage) + ")");
        }
        else
        {
            return "";
        }
    }

    int GetCanDeviceFileDescriptor(const char* canDeviceName)
    {
        struct ifreq ifr;

        int canFileDescriptor = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (canFileDescriptor < 0) 
        {
            int socketCreateErrorCode = errno; // Capture the error code
            _logger->Error("Socket creation failed with error code: " + to_string(socketCreateErrorCode) + extractErrorMessage(socketCreateErrorCode));
            close(canFileDescriptor);
            return FILE_DESCRIPTOR_ERROR;
        }

        // Check if canDeviceName is null or too long, IFNAMSIZ is a constant that defines the maximum possible buffer size for an interface name (including its terminating zero byte)
        if (canDeviceName == nullptr || strlen(canDeviceName) >= IFNAMSIZ)
        {
            _logger->Error("Invalid vCAN device name used for [--can-name] arg.\n" 
                "(Hint): Ensure that the name provided is within a valid length between (1 and " + to_string(IFNAMSIZ-1) + ") characters.");
            close(canFileDescriptor);
            return FILE_DESCRIPTOR_ERROR;
        }

        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, canDeviceName, IFNAMSIZ); 
        
        int errorCode = ioctl(canFileDescriptor, SIOCGIFINDEX, reinterpret_cast<void*>(&ifr));
        if (errorCode < 0)
        {
            int ioctlError = errno;
            _logger->Error("Failed to execute IOCTL system call with error code: " + to_string(ioctlError)+ extractErrorMessage(ioctlError)
                           + "\n(Hint): Ensure that the network interface \"" + std::string(canDeviceName)
                           + "\" specified in [--can-name] exists and is operational.");
            close(canFileDescriptor);
            return FILE_DESCRIPTOR_ERROR;
        }
        
        struct sockaddr_can socketAddress;
        socketAddress.can_family = AF_CAN;
        socketAddress.can_ifindex = ifr.ifr_ifindex;

        // Bind socket (link socketCAN File Descriptor to address)
        /* Give the socket FD the local address ADDR (which is LEN bytes long).  */
        errorCode = bind(canFileDescriptor, (struct sockaddr*)&socketAddress, sizeof(socketAddress));
        if (errorCode < 0)
        {
            int bindErrorCode = errno; // Capture the error code
            _logger->Error("Bind failed with error code: " + to_string(bindErrorCode) + extractErrorMessage(bindErrorCode));
            close(canFileDescriptor);
            return FILE_DESCRIPTOR_ERROR;
        }

        _logger->Info("vCAN device [" + std::string(canDeviceName) + "] successfully opened");


        return canFileDescriptor;
    }

private:
    void ReceiveCanFrameFromVirtualCanDevice()
    {
        _canDeviceStream.async_read_some(asio::buffer(&_canFrameBuffer, sizeof(_canFrameBuffer)),

            [this](const std::error_code& ec, const std::size_t bytes_received) {
                try
                {
                    if (ec)
                    {
                        std::string SILKitErrorMessage = "Unable to receive data from vCAN device.\n" 
                                                "Error code: " + std::to_string(ec.value()) + " (" + ec.message() + ")\n"
                                                "Error category: " + ec.category().name();
                        _logger->Error(SILKitErrorMessage);
                    }
                    else
                    {
                        auto frame_data = std::vector<std::uint8_t>(bytes_received);
                        can_frame CF;
                        asio::buffer_copy(asio::buffer(&CF, sizeof(CF)), asio::buffer(&_canFrameBuffer, sizeof(_canFrameBuffer)), bytes_received);
                        _onNewFrameHandler(std::move(CF));
                    }
                }
                catch (const std::exception& ex)
                {
                    // Handle any exception that might occur
                    std::string SILKitErrorMessage = "Exception occurred: " + std::string(ex.what());
                    _logger->Error(SILKitErrorMessage);
                }
                // Continue with the next read
                ReceiveCanFrameFromVirtualCanDevice();
            });
    }

private:
    asio::posix::basic_stream_descriptor<> _canDeviceStream;
    struct can_frame _canFrameBuffer;
    std::function<void(can_frame)> _onNewFrameHandler;
    SilKit::Services::Logging::ILogger* _logger;
    int _fileDescriptor;
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

void promptForExit()
{
    std::promise<int> signalPromise;
    auto signalValue = signalPromise.get_future();
    RegisterSignalHandler([&signalPromise](auto sigNum) {
        signalPromise.set_value(sigNum);
    });
    
    std::cout << "Press CTRL + C to stop the process..." << std::endl;

    signalValue.wait();

    std::cout << "\nSignal " << signalValue.get() << " received!" << std::endl;
    std::cout << "Exiting..." << std::endl;
}

int main(int argc, char** argv)
{
    if (findArg(argc, argv, helpArg, argv) != NULL)
    {
        print_help(true);
        return NO_ERROR;
    }

    const std::string configurationFile = getArgDefault(argc, argv, configurationArg, "");
    const std::string registryURI = getArgDefault(argc, argv, regUriArg, "silkit://localhost:8501");
    const std::string participantName = getArgDefault(argc, argv, participantNameArg, "SilKitAdapterSocketCAN");

    const std::string canDevName = getArgDefault(argc, argv, canNameArg, "can0");
    const std::string canNetworkName = getArgDefault(argc, argv, networkArg, "CAN1");

    asio::io_context io_context;

    try
    {
        throwInvalidCliIf(thereAreUnknownArguments(argc, argv));

        std::shared_ptr<SilKit::Config::IParticipantConfiguration> participantConfiguration;

        if (!configurationFile.empty())
        {
            participantConfiguration = SilKit::Config::ParticipantConfigurationFromFile(configurationFile);

            static const auto conflictualArguments = {
                &logLevelArg,
                /* others are correctly handled by SilKit if one is overwritten.*/};

            for (const auto* conflictualArgument : conflictualArguments)
            {
                if (findArg(argc, argv, *conflictualArgument, argv) != NULL)
                {
                    auto configFileName = configurationFile;
                    if (configurationFile.find_last_of("/\\") != std::string::npos)
                    {
                        configFileName = configurationFile.substr(configurationFile.find_last_of("/\\") + 1);
                    }
                    std::cout << "[info] Be aware that argument given with " << *conflictualArgument
                              << " can be overwritten by a different value defined in the given configuration file "
                              << configFileName << std::endl;
                }
            }
        }
        else
        {
            const std::string loglevel = getArgDefault(argc, argv, logLevelArg, "Info");
            const std::string participantConfigurationString =
                R"({ "Logging": { "Sinks": [ { "Type": "Stdout", "Level": ")" + loglevel + R"("} ] } })";
            participantConfiguration = SilKit::Config::ParticipantConfigurationFromString(participantConfigurationString);
        }

        const std::string canControllerName = participantName + "_CAN_CTRL";

        auto participant = SilKit::CreateParticipant(participantConfiguration, participantName, registryURI);

        auto logger = participant->GetLogger();

        auto* lifecycleService = participant->CreateLifecycleService({OperationMode::Autonomous});
        auto* systemMonitor = participant->CreateSystemMonitor();
        std::promise<void> runningStatePromise;

        systemMonitor->AddParticipantStatusHandler(
            [&runningStatePromise, participantName](const SilKit::Services::Orchestration::ParticipantStatus& status) {
                if (participantName == status.participantName)
                {
                    if (status.state == SilKit::Services::Orchestration::ParticipantState::Running)
                    {
                        runningStatePromise.set_value();
                    }
                }
            });

        std::ostringstream SILKitInfoMessage;
        SILKitInfoMessage << "Creating CAN controller '" << canControllerName << "'";
        logger->Info(SILKitInfoMessage.str());
        auto* canController = participant->CreateCanController(canControllerName, canNetworkName);

        const auto onReceiveCanFrameFromCanDevice = [&logger, canController](can_frame data) {
            static intptr_t transmitId = 0;
            canController->SendFrame(SocketCANToSILKit(data), reinterpret_cast<void*>(transmitId++));

            std::ostringstream SILKitDebugMessage;

            SILKitDebugMessage << "CAN device >> SIL Kit: CAN frame (dlc=" <<  static_cast<int>(data.can_dlc) << ", CAN ID=0x"
                               << std::hex << static_cast<int>(data.can_id) << std::dec << ", txId=" << transmitId << ")";
            logger->Debug(SILKitDebugMessage.str());
        };

        CanConnection canConnection{io_context, canDevName, onReceiveCanFrameFromCanDevice, logger};

        SILKitInfoMessage.str("");
        SILKitInfoMessage << "Created CAN device connector for [" << canDevName << "] on network [" << canNetworkName << "]";
        logger->Info(SILKitInfoMessage.str());

        const auto onReceiveCanMessageFromSilKit = [&logger, &canConnection](ICanController* /*controller*/,
                                                                             const CanFrameEvent& msg) {
            CanFrame recievedFrame = msg.frame;
            canConnection.SendCanFrameToCanDevice(SILKitToSocketCAN(recievedFrame));

            std::ostringstream SILKitDebugMessage;
            SILKitDebugMessage << "SIL Kit >> CAN device: CAN frame (" << recievedFrame.dlc << " bytes)";
            logger->Debug(SILKitDebugMessage.str());
        };

        const auto onCanAckCallback = [&logger](ICanController* /*controller*/, const CanFrameTransmitEvent& ack) {
            std::ostringstream SILKitDebugMessage;
            if (ack.status == CanTransmitStatus::Transmitted)
            {
                SILKitDebugMessage << "SIL Kit >> CAN : ACK for CAN Message with transmitId=" << reinterpret_cast<intptr_t>(ack.userContext);
            }
            else
            {
                SILKitDebugMessage << "SIL Kit >> CAN : NACK for CAN Message with transmitId=" << reinterpret_cast<intptr_t>(ack.userContext) << ": " << ack.status;
            }
            logger->Debug(SILKitDebugMessage.str());
        };

        canController->AddFrameHandler(onReceiveCanMessageFromSilKit);
        canController->AddFrameTransmitHandler(onCanAckCallback);
        canController->Start();

        auto finalStateFuture = lifecycleService->StartLifecycle();

        std::thread t([&]() -> void {
            io_context.run();
        });

        promptForExit();

        io_context.stop();
        t.join();

        auto runningStateFuture = runningStatePromise.get_future();
        auto futureStatus = runningStateFuture.wait_for(15s);
        if (futureStatus != std::future_status::ready)
        {
            logger->Debug("Lifecycle Service Stopping: timed out while checking if the participant is currently running.");            
        }

        lifecycleService->Stop("Adapter stopped by the user.");

        auto finalState = finalStateFuture.wait_for(15s);
        if (finalState != std::future_status::ready)
        {
             logger->Debug("Lifecycle service stopping: timed out");            
        }
    }
    catch (const SilKit::ConfigurationError& error)
    {
        std::cerr << "Invalid configuration: " << error.what() << std::endl;        
        return CONFIGURATION_ERROR;
    }
    catch (const InvalidCli&)
    {
        adapters::print_help();
        std::cerr << std::endl << "Invalid command line arguments." << std::endl;        
        return CLI_ERROR;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Something went wrong: " << error.what() << std::endl;        
        return OTHER_ERROR;
    }

    return NO_ERROR;
}