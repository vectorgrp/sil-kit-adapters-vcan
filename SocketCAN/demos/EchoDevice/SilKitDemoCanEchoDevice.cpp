// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include <iostream>
#include <string>
#include <cstring>

#include "silkit/SilKit.hpp"
#include "silkit/config/all.hpp"
#include "silkit/services/can/all.hpp"
#include "silkit/services/can/string_utils.hpp"
#include "silkit/util/Span.hpp"

#include "common/Parsing.hpp"
#include "common/Cli.hpp"
#include "common/SignalHandler.hpp"
#include "common/ParticipantCreation.hpp"
#include "adapter/AdapterUtils.hpp"


using namespace SilKit::Services::Can;
using namespace adapters;
using namespace util;

namespace adapters {
const std::string networkArg = "--network";
}


class Device
{
public: 
    Device(const std::string& canDevName, const std::string& canNetName, std::function<void(CanFrame)> sendFrameCallback) : _sendFrameCallback(std::move(sendFrameCallback))
    {
    }

public:

    // set CAN FD dlc field as a non-linear function of size of data field
    // see https://elearning.vector.com/mod/page/view.php?id=368

    void Process(CanFrame incomingData)
    {
        CanFrame canEchoFrame{};
        // assign an ID to the echo frame (received ID + 1)
        canEchoFrame = incomingData;
        canEchoFrame.canId = incomingData.canId+1;
        // silkit frame -> char array -> shift array -> silkit frame
        const unsigned int frameSize = incomingData.dataField.size();
        unsigned char tmpFrame[frameSize];
        memcpy(tmpFrame, incomingData.dataField.data(), frameSize);
        shift_left_by_one(tmpFrame, frameSize);
        canEchoFrame.dataField = SilKit::Util::Span<const uint8_t>(tmpFrame, frameSize);
        canEchoFrame.dlc = AdapterUtils::CalculateDLC(incomingData.dataField.size());
        canEchoFrame.flags = incomingData.flags;
        _sendFrameCallback(std::move(canEchoFrame));
    }
    void shift_left_by_one(unsigned char* arr, size_t n)
    {
        try
        {
            if (n > 0 && arr != nullptr)
            {
                std::memmove(arr, &arr[1], n * sizeof(unsigned char));
                // set right byte to 0x00
                arr[n - 1] = 0x00;
            }
        }
        catch (const std::exception& error)
        {
            std::cerr << "Something went wrong: " << error.what() << std::endl;            
        }
    }
    
private: 
    std::function<void(CanFrame)> _sendFrameCallback;
};

void print_demo_help(bool userRequested)
{
    std::cout << "Usage (defaults in curly braces if you omit the switch):" << std::endl;
    std::cout << "sil-kit-demo-can-echo-device [" << participantNameArg << " <participant's name{CanEchoDevice}>]\n"
        "  [" << regUriArg << " silkit://<host{localhost}>:<port{8501}>]\n"
        "  [" << networkArg << " <SIL Kit CAN network name{CAN1}>]\n"
        "  [" << logLevelArg << " <Trace|Debug|Warn|{Info}|Error|Critical|Off>]\n";
        std::cout << "\n"
        "Example:\n"
        "sil-kit-demo-can-echo-device " << participantNameArg << " EchoDevice " << networkArg << " CAN_NETWORK " << logLevelArg << " Off\n ";

    if (!userRequested)
        std::cout << "\n"
            "Pass "<<helpArg<<" to get this message.\n";
}

 /**************************************************************************************************
 * Main Function
 **************************************************************************************************/

int main(int argc, char** argv)
{
    if (findArg(argc, argv, helpArg, argv) != nullptr)
    {
        print_demo_help(true);
        return CodeSuccess;
    }

    try
    {
        throwInvalidCliIf(thereAreUnknownArguments(
            argc, argv, {&logLevelArg, &participantNameArg, &regUriArg, &networkArg}, {&helpArg}));

        const std::string loglevel = getArgDefault(argc, argv, logLevelArg, "Info");
        const std::string registryURI = getArgDefault(argc, argv, regUriArg, "silkit://localhost:8501");
        const std::string canNetworkName = getArgDefault(argc, argv, networkArg, "CAN1");
        std::string participantName = getArgDefault(argc, argv, participantNameArg, "CanEchoDevice");

        const std::string canControllerName = participantName + "_CAN1";
        const std::string participantConfigurationString =
            R"({ "Logging": { "Sinks": [ { "Type": "Stdout", "Level": ")" + loglevel + R"("} ] } })";

        auto participantConfiguration = SilKit::Config::ParticipantConfigurationFromString(participantConfigurationString);

        SilKit::Services::Logging::ILogger* logger;
        auto participant = CreateParticipant(argc, argv, logger, &participantName);

        logger->Info("Creating CAN controller '" + canControllerName + "'");
        auto* canController = participant->CreateCanController(canControllerName, canNetworkName);

        auto demoDevice = Device{ canControllerName, canNetworkName,[&logger, canController](CanFrame data) 
                                {
                                    static intptr_t transmitId = 0;
                                    canController->SendFrame(CanFrame{ std::move(data) }, reinterpret_cast<void*>(++transmitId));
                                    
                                    std::ostringstream SILKitDebugMessage;
                                    SILKitDebugMessage << "Demo >> SIL Kit : CAN frame (dlc=" << (int)data.dlc << ", txId=" << transmitId << ")";
                                    logger->Debug(SILKitDebugMessage.str());
                                }};

        auto onReceiveCanMessageFromSilKit = [&logger, &demoDevice](ICanController* /*controller*/, const CanFrameEvent& msg) 
        {
            std::ostringstream SILKitDebugMessage;

            SILKitDebugMessage << "SIL Kit >> Demo: CAN frame (dlc=" << msg.frame.dlc << ")";
            logger->Debug(SILKitDebugMessage.str());
            demoDevice.Process(std::move(msg.frame));
        };

        const auto onCanAckCallback = [&logger](ICanController* /*controller*/, const CanFrameTransmitEvent& ack) {
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
            logger->Debug(SILKitDebugMessage.str());
        };

        canController->AddFrameHandler(onReceiveCanMessageFromSilKit);
        canController->AddFrameTransmitHandler(onCanAckCallback);
        canController->Start();

        promptForExit();
    }
    catch (const SilKit::ConfigurationError& error)
    {
        std::cerr << "Invalid configuration: " << error.what() << std::endl;        
        return CodeErrorConfiguration;
    }
    catch (const InvalidCli&)
    {
        print_demo_help(false);
        std::cerr << std::endl << "Invalid command line arguments." << std::endl;        
        return CodeErrorCli;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Something went wrong: " << error.what() << std::endl;        
        return CodeErrorOther;
    }
    return CodeSuccess;
}

