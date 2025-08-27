// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include "ConnectionsImplementations.hpp"
#include "AdapterConnections.hpp"
#include "AdapterUtils.hpp"

#include "silkit/services/logging/all.hpp"

using namespace AdapterUtils;

///////////////////////////////////////////////////////////
///   ICanConnectionImpl functions implementations      ///
///////////////////////////////////////////////////////////
void ICanConnectionImpl::HandleReceivedCanFrameFromVirtualCanDevice(ICanController* canController, SilKit::Services::Logging::ILogger* logger)
{
    CanFrame frame = SocketCANToSILKit();
    try
    {
        static intptr_t transmitId = 0;
        bool frame_is_FD = (frame.flags & SilKit_CanFrameFlag_fdf) && (frame.flags & SilKit_CanFrameFlag_brs);

        canController->SendFrame(frame, reinterpret_cast<void*>(transmitId++));

        std::ostringstream SILKitDebugMessage;
        SILKitDebugMessage << "CAN device >> SIL Kit:" << (frame_is_FD ? " CAN FD frame " : " CAN frame ")
                           << "(payload = " << static_cast<int>(frame.dataField.size()) << " [bytes] , CAN ID=0x"
                           << std::hex << static_cast<int>(frame.canId) << std::dec << ", txId=" << transmitId << ")";
        logger->Debug(SILKitDebugMessage.str());
    }
    catch (const std::exception& e)
    {
        // Handle the exception
        logger->Error("Exception occurred: " + std::string(e.what()));
    }
}

/////////////////////////////////////////////////////////////
///  ClassicalCanConnectionImpl functions implementations ///
/////////////////////////////////////////////////////////////

// Convert a ClassicalCanConnectionImpl to a SilKit CanFrame
CanFrame ClassicalCanConnectionImpl::SocketCANToSILKit()
{
    CanFrame silkit_frame = CanFrame{};
    // initialise all flags to 0
    silkit_frame.flags = 0;

    if (_frameToSilKit.can_id & CAN_EFF_FLAG)
    {
        // extended frame format / identifier extension (29 bit CAN ID)
        silkit_frame.canId = _frameToSilKit.can_id & CAN_EFF_MASK;
        silkit_frame.flags |= static_cast<CanFrameFlagMask>(CanFrameFlag::Ide);
    }
    else
    {
        // standard frame format / no identifier extension (11 bit CAN ID)
        silkit_frame.canId = _frameToSilKit.can_id & CAN_SFF_MASK;
    }

    if (_frameToSilKit.can_id & CAN_RTR_FLAG)
    {
        silkit_frame.flags |= static_cast<CanFrameFlagMask>(CanFrameFlag::Rtr);
    }

    if (_frameToSilKit.can_id & CAN_ERR_FLAG)
    {
        // SIL Kit does not support error frames
        throw adapters::UnsupportedCANFrame();
    }

    silkit_frame.dlc = _frameToSilKit.can_dlc;
    silkit_frame.sdt = 0; // not used in SocketCAN
    silkit_frame.vcid = 0; // not used in SocketCAN
    silkit_frame.af = 0; // not used in SocketCAN
    silkit_frame.dataField = SilKit::Util::Span<const uint8_t>(_frameToSilKit.data, _frameToSilKit.can_dlc);

    return silkit_frame;
}

// Writes the SIL Kit CanFrame to the stream_descriptor
void ClassicalCanConnectionImpl:: WriteToStream(asio::posix::stream_descriptor* stream, const CanFrame& SilkitFrame)
{
    bool silkit_frame_is_fd = (SilkitFrame.flags & SilKit_CanFrameFlag_brs) && (SilkitFrame.flags & SilKit_CanFrameFlag_fdf);
    // if the received SILKit frame is a Classical CAN frame : OK. Invalid frame otherwise.
    if(silkit_frame_is_fd)
    {
        _frameType = INVALID_FRAME;
        throw adapters::UnsupportedCANFrame();
    }
    else
    {
        // set all fields accordingly
        _frameToVCAN.can_id = SilkitFrame.canId;
        if (SilkitFrame.flags & static_cast<CanFrameFlagMask>(CanFrameFlag::Ide))
        {
            _frameToVCAN.can_id |= CAN_EFF_FLAG;
        }
        if (SilkitFrame.flags & static_cast<CanFrameFlagMask>(CanFrameFlag::Rtr))
        {
            _frameToVCAN.can_id |= CAN_RTR_FLAG;
        }
        _frameToVCAN.can_dlc = SilkitFrame.dlc;
        memset(& _frameToVCAN.__pad, 0, sizeof( _frameToVCAN.__pad)); // set padding to zero
        memset(& _frameToVCAN.__res0, 0, sizeof( _frameToVCAN.__res0)); // set reserved field to zero
        memcpy( _frameToVCAN.data, SilkitFrame.dataField.data(), SilkitFrame.dataField.size());
        _frameType = PURE_CLASSIC_FRAME;
    }

    size_t sizeSent = stream->write_some(asio::mutable_buffer(&_frameToVCAN, sizeof(_frameToVCAN)));

    if (sizeSent != sizeof(can_frame))
    {
        throw adapters::InvalidFrameSizeError{};
    }
};

///////////////////////////////////////////////////////////
///         CanFDConnectionImpl functions implementations        ///
///////////////////////////////////////////////////////////

// Convert a CanFDConnectionImpl to a SilKit CanFrame
CanFrame CanFDConnectionImpl::SocketCANToSILKit()
{
    CanFrame silkit_frame = CanFrame{};
    // initialise all flags to 0
    silkit_frame.flags = 0;

    if(_frameType == PURE_FD_FRAME)
    {
        // set CAN FD related flags in the SIL Kit CAN frame
        silkit_frame.flags = static_cast<CanFrameFlagMask>(CanFrameFlag::Fdf) // FD Format Indicator
                             | static_cast<CanFrameFlagMask>(CanFrameFlag::Brs); // Bit Rate Switch (for FD Format only)
    }

    if (_frameToSilKit.can_id & CAN_EFF_FLAG)
    {
        // extended frame format / identifier extension (29 bit CAN ID)
        silkit_frame.canId = _frameToSilKit.can_id & CAN_EFF_MASK;
        silkit_frame.flags |= static_cast<CanFrameFlagMask>(CanFrameFlag::Ide);
    }
    else
    {
        // standard frame format / no identifier extension (11 bit CAN ID)
        silkit_frame.canId = _frameToSilKit.can_id & CAN_SFF_MASK;
    }

    if (_frameToSilKit.can_id & CAN_RTR_FLAG)
    {
        silkit_frame.flags |= static_cast<CanFrameFlagMask>(CanFrameFlag::Rtr);
    }

    if (_frameToSilKit.can_id & CAN_ERR_FLAG)
    {
        // SIL Kit does not support error frames
        throw adapters::UnsupportedCANFrame();
    }

    silkit_frame.sdt = 0; // set to 0 because this field is for XL format only
    silkit_frame.vcid = 0; // set to 0 because this field is for XL format only
    silkit_frame.af = 0; // set to 0 because this field is for XL format only
    silkit_frame.dataField = SilKit::Util::Span<const uint8_t>( _frameToSilKit.data,  _frameToSilKit.len);
    silkit_frame.dlc = AdapterUtils::CalculateDLC( _frameToSilKit.len);
    return silkit_frame;
};

// Writes the SIL Kit CanFrame to the stream_descriptor
void CanFDConnectionImpl::WriteToStream(asio::posix::stream_descriptor* stream, const CanFrame& SilkitFrame)
{
    _frameToVCAN.can_id = SilkitFrame.canId;
    if (SilkitFrame.flags & static_cast<CanFrameFlagMask>(CanFrameFlag::Ide))
    {
        _frameToVCAN.can_id |= CAN_EFF_FLAG;
    }

    if (SilkitFrame.flags & static_cast<CanFrameFlagMask>(CanFrameFlag::Rtr))
    {
        _frameToVCAN.can_id |= CAN_RTR_FLAG;
    }
    memset(& _frameToVCAN.__res0, 0, sizeof( _frameToVCAN.__res0));
    memset(& _frameToVCAN.__res1, 0, sizeof( _frameToVCAN.__res1));
    memcpy( _frameToVCAN.data, SilkitFrame.dataField.data(), SilkitFrame.dataField.size());
    // check if frame is a CAN FD frame
    size_t sizeSent;
    if((SilkitFrame.flags & SilKit_CanFrameFlag_brs) && (SilkitFrame.flags & SilKit_CanFrameFlag_fdf))
    {
        // Set the frame length based on whether it's a CAN FD frame or a classical CAN frame
        _frameToVCAN.flags = CANFD_BRS; // Bit Rate Switch
        _frameToVCAN.len = SilkitFrame.dataField.size();
        sizeSent = stream->write_some(asio::mutable_buffer(&_frameToVCAN, sizeof(_frameToVCAN)));
        if (sizeSent != sizeof(canfd_frame))
        {
            throw adapters::InvalidFrameSizeError{};
        }
    }
    else
    {
        _frameToVCAN.flags = 0;
        _frameToVCAN.len = SilkitFrame.dlc;
        sizeSent = stream->write_some(asio::mutable_buffer(&_frameToVCAN, sizeof(can_frame)));
        if (sizeSent != sizeof(can_frame))
        {
            throw adapters::InvalidFrameSizeError{};
        }
    }
};

inline can_frame SILKitToSocketCAN(const CanFrame& silkit_can_frame)
{
    struct can_frame socketcan_frame;
    socketcan_frame.can_id = silkit_can_frame.canId;

    if (silkit_can_frame.flags & static_cast<CanFrameFlagMask>(CanFrameFlag::Ide))
    {
        socketcan_frame.can_id |= CAN_EFF_FLAG;
    }

    if (silkit_can_frame.flags & static_cast<CanFrameFlagMask>(CanFrameFlag::Rtr))
    {
        socketcan_frame.can_id |= CAN_RTR_FLAG;
    }

    socketcan_frame.can_dlc = silkit_can_frame.dlc;
    memset(&socketcan_frame.__pad, 0, sizeof(socketcan_frame.__pad)); // set padding to zero
    memset(&socketcan_frame.__res0, 0, sizeof(socketcan_frame.__res0)); // set reserved field to zero
    //socketcan_frame.len8_dlc = silkit_can_frame.dlc; // optional DLC for 8 byte payload length (9 .. 15)
    memcpy(socketcan_frame.data, silkit_can_frame.dataField.data(), silkit_can_frame.dataField.size());
    return socketcan_frame;
}

