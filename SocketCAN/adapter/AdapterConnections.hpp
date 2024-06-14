// Copyright (c) Vector Informatik GmbH. All rights reserved.
#pragma once

#include <memory>

#include "Exceptions.hpp"
#include "AdapterUtils.hpp"
#include "ConnectionsImplementations.hpp"

#include "asio/posix/stream_descriptor.hpp"
#include "silkit/services/can/all.hpp"
#include "silkit/services/can/string_utils.hpp"

using namespace AdapterUtils;
using namespace exceptions;
using namespace std;
using namespace SilKit::Services::Can;

// Implements the connection with the vCAN device
class CanConnection : public std::enable_shared_from_this<CanConnection>
{
private:
    CanConnection(asio::io_context& io_context,  ICanController* silkitCtrl, SilKit::Services::Logging::ILogger* logger, const char* canDeviceName);
    CanConnection(const CanConnection&) = delete;

public:
    static shared_ptr<CanConnection> Create(asio::io_context& io_context, ICanController* silkitCtrl, SilKit::Services::Logging::ILogger* logger, const char* canDeviceName);
    void SendSILKitCanFrameToVirtualCanDevice(const CanFrame& SilkitFrame);
    void ReceiveCanFrameFromVirtualCanDevice();

private:
    // Interrogates vCAN device "canDeviceName", binds to it and sets the _vcanDevice member accordingly
    void InitialiseVirtualCANConnection(const char* canDeviceName);
    static void InitialiseSILKitCANConnection(ICanController* silkitCtrl, const shared_ptr<CanConnection>& ptr);

private:
    asio::posix::stream_descriptor _canDeviceStream;
    std::unique_ptr<ICanConnectionImpl> _canConnectionImpl;
    SilKit::Services::Logging::ILogger* _logger;
    ICanController* _silkitCtrl;

    struct VCANDevice
    {
        int fileDescriptor;
        vCANDeviceType deviceType;
    } _vcanDevice;
};
