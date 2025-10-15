// SPDX-FileCopyrightText: Copyright 2025 Vector Informatik GmbH
// SPDX-License-Identifier: MIT
#pragma once

#include <memory>

#include "ConnectionsImplementations.hpp"
#include "common/Exceptions.hpp"

#include "silkit/services/can/all.hpp"

using namespace std;
using namespace SilKit::Services::Can;

class ICanConnectionImpl;

// Implements the connection with the vcan device
class CanConnection : public std::enable_shared_from_this<CanConnection>
{
private:
    CanConnection(asio::io_context& io_context, ICanController* silkitCtrl, SilKit::Services::Logging::ILogger* logger,
                  const char* canDeviceName);
    CanConnection(const CanConnection&) = delete;

public:
    static shared_ptr<CanConnection> Create(asio::io_context& io_context, ICanController* silkitCtrl,
                                            SilKit::Services::Logging::ILogger* logger, const char* canDeviceName);
    void SendSILKitCanFrameToVirtualCanDevice(const CanFrame& SilkitFrame);
    void ReceiveCanFrameFromVirtualCanDevice();

private:
    // Interrogates vcan device "canDeviceName", binds to it and sets the _vcanDevice member accordingly
    void InitialiseVirtualCANConnection(const char* canDeviceName);
    static void InitialiseSILKitCANConnection(ICanController* silkitCtrl, const shared_ptr<CanConnection>& ptr);

private:
    asio::posix::stream_descriptor _canDeviceStream;
    std::unique_ptr<ICanConnectionImpl> _canConnectionImpl;
    SilKit::Services::Logging::ILogger* _logger;
    ICanController* _silkitCtrl;

    struct CanDevice
    {
        int fileDescriptor;
        CanDeviceType deviceType;
    } _vcanDevice;
};

namespace adapters {

struct UnsupportedCANFrame : public std::runtime_error
{
    UnsupportedCANFrame()
        : std::runtime_error("CAN frame is not supported by the connected vcan device.")
    {
    }
};

struct InvalidVirtualCANDevice : public std::runtime_error
{
    InvalidVirtualCANDevice()
        : std::runtime_error("An invalid or not supported virtual CAN device has been passed to the adapter.")
    {
    }
};

inline auto& throwInvalidFileDescriptorIf = throwIf<InvalidVirtualCANDevice>;
inline auto& throwInvalidCANFrameIf = throwIf<UnsupportedCANFrame>;

} // namespace adapters
