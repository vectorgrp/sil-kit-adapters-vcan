// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once

#include <linux/can.h>
#include <linux/can/raw.h>
#include <iostream>
#include <sstream>
#include <memory>

#include "AdapterUtils.hpp"
#include "Exceptions.hpp"

#include "asio/ts/io_context.hpp"
#include "asio/ts/net.hpp"
#include "asio/posix/stream_descriptor.hpp"
#include "silkit/services/can/ICanController.hpp"
#include "silkit/services/logging/ILogger.hpp"
#include "silkit/services/can/CanDatatypes.hpp"

using namespace SilKit::Services::Can;

enum vCANDeviceType
{
    CLASSICAL_CAN_DEVICE,
    CAN_FD_DEVICE,
    INVALID_DEVICE
};

enum canFrameType
{
    PURE_FD_FRAME,
    FD_CLASSIC_FRAME,
    PURE_CLASSIC_FRAME,
    INVALID_FRAME
};

struct ICanConnectionImpl
{
    virtual ~ICanConnectionImpl() = default;
    virtual void UpdateFrameType(const std::size_t bytes_received) = 0;
    virtual CanFrame SocketCANToSILKit() = 0;
    virtual asio::mutable_buffer GetBuffer() = 0;
    // Writes the SIL Kit CanFrame to the stream_descriptor
    virtual void WriteToStream(asio::posix::stream_descriptor* stream, const CanFrame& SilkitFrame) = 0;
    // A function that is used as a callback that handles received frames from the vcan device, sending them on the SIL Kit network
    void HandleReceivedCanFrameFromVirtualCanDevice(ICanController* canController, SilKit::Services::Logging::ILogger* logger);

protected:
    canFrameType _frameType;
};

struct ClassicalCanConnectionImpl : public ICanConnectionImpl
{
public:
    // Default constructor for ClassicalCanConnectionImpl
    ClassicalCanConnectionImpl() = default;
    // Update the _frameType attribute
    void UpdateFrameType(const std::size_t bytes_received) override { _frameType = PURE_CLASSIC_FRAME; };
    // Convert an instance of ClassicalCanConnectionImpl into a SIL Kit CanFrame
    CanFrame SocketCANToSILKit() override;
    // Returns an asio mutable buffer from the ClassicalCanConnectionImpl
    asio::mutable_buffer GetBuffer() override { return asio::buffer(&_frameToSilKit, sizeof(_frameToSilKit)); };
    // Writes the SIL Kit CanFrame to the stream_descriptor
    void WriteToStream(asio::posix::stream_descriptor* stream, const CanFrame& SilkitFrame) override;

private:
    can_frame _frameToVCAN, _frameToSilKit;
};

struct CanFDConnectionImpl : public ICanConnectionImpl
{
public:
    // Default constructor for ClassicalCanConnectionImpl
    CanFDConnectionImpl() = default;
    // Update the _frameType attribute
    void UpdateFrameType(const std::size_t bytes_received) override { _frameType = (bytes_received == CANFD_MTU)? PURE_FD_FRAME : FD_CLASSIC_FRAME;};
    // Convert an instance of CanFDConnectionImpl into a SIL Kit CanFrame
    CanFrame SocketCANToSILKit() override;
    // Returns an asio mutable buffer from the CanFDConnectionImpl
    asio::mutable_buffer GetBuffer() override { return asio::buffer(&_frameToSilKit, sizeof(_frameToSilKit)); };
    // Writes the SIL Kit CanFrame to the stream_descriptor
    void WriteToStream(asio::posix::stream_descriptor* stream, const CanFrame& SilkitFrame) override;

private:
    canfd_frame _frameToVCAN, _frameToSilKit;
};