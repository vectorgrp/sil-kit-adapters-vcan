// SPDX-FileCopyrightText: Copyright 2025 Vector Informatik GmbH
// SPDX-License-Identifier: MIT

#include "AdapterConnections.hpp"
#include "AdapterUtils.hpp"
#include "ConnectionsImplementations.hpp"

#include "silkit/services/can/string_utils.hpp"
#include "silkit/services/logging/all.hpp"

using namespace adapters;
using namespace AdapterUtils;

CanConnection::CanConnection(asio::io_context& io_context, ICanController* silkitCtrl,
                             SilKit::Services::Logging::ILogger* logger, const char* canDeviceName)
    : _canDeviceStream{io_context}
    , _silkitCtrl(silkitCtrl)
    , _logger(logger)
    , _vcanDevice{-1, INVALID_DEVICE}
{
    InitialiseVirtualCANConnection(canDeviceName);
    _canDeviceStream.assign(_vcanDevice.fileDescriptor);

    // instantiate buffer according to device type
    switch (_vcanDevice.deviceType)
    {
    case CAN_FD_DEVICE:
    {
        _canConnectionImpl = make_unique<CanFDConnectionImpl>();
        break;
    }
    case CLASSICAL_CAN_DEVICE:
    {
        _canConnectionImpl = make_unique<ClassicalCanConnectionImpl>();
        break;
    }
    default:
    {
        throw InvalidVirtualCANDevice();
    }
    }
}

void CanConnection::SendSILKitCanFrameToVirtualCanDevice(const CanFrame& SilkitFrame)
{
    _canConnectionImpl->WriteToStream(&_canDeviceStream, SilkitFrame);
}

void CanConnection::ReceiveCanFrameFromVirtualCanDevice()
{
    _canDeviceStream.async_read_some(
        _canConnectionImpl->GetBuffer(),
        [that = shared_from_this()](const std::error_code& ec, const std::size_t bytes_received) {
        try
        {
            if (ec)
            {
                std::string SILKitErrorMessage = "Unable to receive data from vcan device.\n"
                                                 "Error code: "
                                                 + std::to_string(ec.value()) + " (" + ec.message()
                                                 + ")\n"
                                                   "Error category: "
                                                 + ec.category().name();
                that->_logger->Error(SILKitErrorMessage);
            }
            else
            {
                that->_canConnectionImpl->UpdateFrameType(bytes_received);
                that->_canConnectionImpl->HandleReceivedCanFrameFromVirtualCanDevice(that->_silkitCtrl, that->_logger);
            }
        }
        catch (const std::exception& ex)
        {
            // Handle any exception that might occur
            std::string SILKitErrorMessage = "Exception occurred: " + std::string(ex.what());
            that->_logger->Error(SILKitErrorMessage);
        }
        // Continue with the next read
        that->ReceiveCanFrameFromVirtualCanDevice();
    });
}

// Initialise the connection with the vcan device, return an initialised VCANDevice object.
void CanConnection::InitialiseVirtualCANConnection(const char* canDeviceName)
{
    ifreq ifr{};

    _vcanDevice.fileDescriptor = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (_vcanDevice.fileDescriptor < 0)
    {
        int socketCreateErrorCode = errno; // Capture the error code
        _logger->Error("Socket creation failed with error code: " + to_string(socketCreateErrorCode)
                       + ExtractErrorMessage(socketCreateErrorCode));
        close(_vcanDevice.fileDescriptor);
        throw InvalidVirtualCANDevice();
    }

    // Check if canDeviceName is null or too long, IFNAMSIZ is a constant that defines the maximum possible buffer size for an interface name (including its terminating zero byte)
    if (canDeviceName == nullptr || strlen(canDeviceName) >= IFNAMSIZ)
    {
        _logger->Error("Invalid vcan device name used for [--can-name] arg.\n"
                       "(Hint): Ensure that the name provided is within a valid length between (1 and "
                       + to_string(IFNAMSIZ - 1) + ") characters.");
        close(_vcanDevice.fileDescriptor);
        throw InvalidVirtualCANDevice();
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, canDeviceName, IFNAMSIZ);
    int errorCode = ioctl(_vcanDevice.fileDescriptor, SIOCGIFINDEX, &ifr);
    if (errorCode < 0)
    {
        int ioctlError = errno;
        _logger->Error("Failed to execute IOCTL system call with error code: " + to_string(ioctlError)
                       + ExtractErrorMessage(ioctlError) + "\n(Hint): Ensure that the network interface \""
                       + std::string(canDeviceName) + "\" specified in [--can-name] exists and is operational.");
        close(_vcanDevice.fileDescriptor);
        throw InvalidVirtualCANDevice();
    }

    struct sockaddr_can socketAddress
    {
    };
    socketAddress.can_family = AF_CAN;
    socketAddress.can_ifindex = ifr.ifr_ifindex;

    // Bind socket (link socketCAN File Descriptor to address)
    errorCode = ::bind(_vcanDevice.fileDescriptor, (struct sockaddr*)&socketAddress, sizeof(socketAddress));
    if (errorCode < 0)
    {
        int bindErrorCode = errno; // Capture the error code
        _logger->Error("Bind failed with error code: " + to_string(bindErrorCode) + ExtractErrorMessage(bindErrorCode));
        close(_vcanDevice.fileDescriptor);
        throw InvalidVirtualCANDevice();
    }

    //fetch CAN device MTU value
    errorCode = ioctl(_vcanDevice.fileDescriptor, SIOCGIFMTU, &ifr);
    if (errorCode < 0)
    {
        int ioctlError = errno;
        _logger->Error("Failed to execute IOCTL system call with error code: " + to_string(ioctlError)
                       + ExtractErrorMessage(ioctlError) + "\n(Hint): Ensure that the network interface \""
                       + std::string(canDeviceName) + "\" specified in [--can-name] exists and is operational.");
        close(_vcanDevice.fileDescriptor);
        throw InvalidVirtualCANDevice();
    }

    _vcanDevice.deviceType = (ifr.ifr_mtu == CANFD_MTU) ? CAN_FD_DEVICE : CLASSICAL_CAN_DEVICE;

    if (_vcanDevice.deviceType == CAN_FD_DEVICE)
    {
        // For CAN FD capable devices, keep CAN_RAW_FD_FRAMES enabled so that the application can send both CAN frames and CAN FD frames
        int enable_CAN_RAW_FD_FRAMES = 1; /* 0 = disabled (default), 1 = enabled */
        errorCode = setsockopt(_vcanDevice.fileDescriptor, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable_CAN_RAW_FD_FRAMES,
                               sizeof(enable_CAN_RAW_FD_FRAMES));
        if (errorCode < 0)
        {
            int socketOptError = errno;
            _logger->Error("Enabling CAN FD support on socket failed with error code: " + to_string(socketOptError)
                           + ExtractErrorMessage(socketOptError));
            close(_vcanDevice.fileDescriptor);
            throw InvalidVirtualCANDevice();
        }
    }

    _logger->Info("The used " + std::string(canDeviceName) + " vcan device is"
                  + (_vcanDevice.deviceType == CAN_FD_DEVICE ? "" : " not")
                  + " CAN FD compatible (MTU = " + to_string(ifr.ifr_mtu) + ")");
    _logger->Info("vcan device [" + std::string(canDeviceName) + "] successfully opened");

    throwInvalidFileDescriptorIf((_vcanDevice.fileDescriptor < 0));
}

shared_ptr<CanConnection> CanConnection::Create(asio::io_context& io_context, ICanController* silkitCtrl,
                                                SilKit::Services::Logging::ILogger* logger, const char* canDeviceName)
{
    shared_ptr<CanConnection> ptr(new CanConnection(io_context, silkitCtrl, logger, canDeviceName));
    InitialiseSILKitCANConnection(silkitCtrl, ptr);
    return ptr;
}

void CanConnection::InitialiseSILKitCANConnection(ICanController* silkitCtrl, const shared_ptr<CanConnection>& that)
{
    if (that->_logger->GetLogLevel() > SilKit::Services::Logging::Level::Debug)
    {
        silkitCtrl->AddFrameHandler([that](ICanController* /*controller*/, const CanFrameEvent& msg) {
            that->SendSILKitCanFrameToVirtualCanDevice(msg.frame);
        });
    }
    else
    {
        silkitCtrl->AddFrameHandler([that](ICanController* /*controller*/, const CanFrameEvent& msg) {
            const CanFrame& recievedFrame = msg.frame;
            bool frame_is_fd =
                (recievedFrame.flags & SilKit_CanFrameFlag_brs) && (recievedFrame.flags & SilKit_CanFrameFlag_fdf);
            std::ostringstream SILKitDebugMessage;
            SILKitDebugMessage << "SIL Kit >> CAN device: " << (frame_is_fd ? "CAN FD frame " : "CAN frame ")
                               << "(payload = " << recievedFrame.dataField.size() << " [bytes], CAN ID=0x" << std::hex
                               << static_cast<int>(recievedFrame.canId) << ")";
            that->_logger->Debug(SILKitDebugMessage.str());
            that->SendSILKitCanFrameToVirtualCanDevice(recievedFrame);
        });

        silkitCtrl->AddFrameTransmitHandler([that](ICanController* /*controller*/, const CanFrameTransmitEvent& ack) {
            std::ostringstream SILKitDebugMessage;
            if (ack.status == CanTransmitStatus::Transmitted)
            {
                SILKitDebugMessage << "SIL Kit >> CAN : ACK for CAN Message with transmitId="
                                   << reinterpret_cast<intptr_t>(ack.userContext);
            }
            else
            {
                SILKitDebugMessage << "SIL Kit >> CAN : NACK for CAN Message with transmitId="
                                   << reinterpret_cast<intptr_t>(ack.userContext) << ": " << ack.status;
            }
            that->_logger->Debug(SILKitDebugMessage.str());
        });
    }
}
