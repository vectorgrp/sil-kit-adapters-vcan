// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include <iostream>
#include <string>
#include <vector>
#include <functional>

#include "silkit/SilKit.hpp"
#include "silkit/config/all.hpp"
#include "silkit/services/can/all.hpp"
#include "silkit/services/can/string_utils.hpp"
#include <asio/ts/buffer.hpp>
#include "silkit/util/Span.hpp"
#include "../adapter/Parsing.hpp"
#include "../adapter/SilKitAdapterSocketCAN.hpp"

using namespace SilKit::Services::Can;
using namespace adapters;

class Device
{
public: 
    Device(const std::string& canDevName, const std::string& canNetName, std::function<void(CanFrame)> sendFrameCallback) : _sendFrameCallback(std::move(sendFrameCallback))
    {
    }

public: 
    void Process(CanFrame incomingData)
    {
        CanFrame canEchoFrame{};
        
        // assign an ID to the echo frame (received ID + 1)
        canEchoFrame.canId = incomingData.canId+1;
        // silkit frame -> char array -> shift array -> silkit frame
        const unsigned int frameSize = incomingData.dlc;
        unsigned char tmpFrame[frameSize];
        memcpy(tmpFrame, incomingData.dataField.data(), frameSize);
        shift_left_by_one(tmpFrame, frameSize);
        canEchoFrame.dataField = SilKit::Util::Span<const uint8_t>(tmpFrame, frameSize);
        canEchoFrame.dlc = canEchoFrame.dataField.size();
        canEchoFrame.flags = incomingData.flags;
        _sendFrameCallback(std::move(canEchoFrame));
    }
    void shift_left_by_one(unsigned char* arr, size_t n)
    {
        try
        {
            if (n > 0 && arr != nullptr)
                std::memmove(arr, &arr[1], n * sizeof(unsigned char));
            // set right byte to 0x00
            arr[n - 1] = 0x00;
        }
        catch (const std::exception& error)
        {
            std::cerr << "Something went wrong: " << error.what() << std::endl;
            std::cout << "Press enter to stop the process..." << std::endl;
            std::cin.ignore();
        }
    }
    
private: 
    std::function<void(CanFrame)> _sendFrameCallback;
};

void CanAckCallback(ICanController* /*controller*/, const CanFrameTransmitEvent& ack)
{
    if (ack.status == CanTransmitStatus::Transmitted)
    {
        std::cout << "SIL Kit >> Demo : ACK for CAN Message with transmitId="
            << reinterpret_cast<intptr_t>(ack.userContext) << std::endl;
    }
    else
    {
        std::cout << "SIL Kit >> Demo : NACK for CAN Message with transmitId="
            << reinterpret_cast<intptr_t>(ack.userContext) << ": " << ack.status << std::endl;
    }
}

/**************************************************************************************************
 * Main Function
 **************************************************************************************************/

inline auto& throwInvalidCliIf = throwIf<InvalidCli>;

int main(int argc, char** argv)
{
    if (findArg(argc, argv, "--help", argv) != nullptr)
    {
        std::cout << "Usage (defaults in curly braces if you omit the switch):" << std::endl
                  << "SilKitDemoCanEchoDevice [" << participantNameArg << " <participant's name{CanEchoDevice}>]\n"
                     "  [" << regUriArg << " silkit://<host{localhost}>:<port{8501}>]\n"
                     "  [" << networkArg << " <SIL Kit CAN network name{CAN1}>]\n"
                     "  ["  << logLevelArg << " <Trace|Debug|Warn|{Info}|Error|Critical|off>]\n";
    }

    const std::string loglevel = getArgDefault(argc, argv, logLevelArg, "Info");
    const std::string participantName = getArgDefault(argc, argv, participantNameArg, "CanEchoDevice");
    const std::string registryURI = getArgDefault(argc, argv, regUriArg, "silkit://localhost:8501");
    const std::string canNetworkName = getArgDefault(argc, argv, networkArg, "CAN1");

    const std::string canControllerName = participantName + "_CAN1";
    const std::string participantConfigurationString = R"({ "Logging": { "Sinks": [ { "Type": "Stdout", "Level": ")" + loglevel + R"("} ] } })";

    try
    {
        throwInvalidCliIf(thereAreUnknownArguments(argc, argv));
        auto participantConfiguration = SilKit::Config::ParticipantConfigurationFromString(participantConfigurationString);
        std::cout << "Creating participant '" << participantName << "' at " << registryURI << std::endl;
        auto participant = SilKit::CreateParticipant(participantConfiguration, participantName, registryURI);

        std::cout << "Creating CAN controller '" << canControllerName << "'" << std::endl;
        auto* canController = participant->CreateCanController(canControllerName, canNetworkName);

        auto demoDevice = Device{ canControllerName, canNetworkName,[canController](CanFrame data) 
                                {
                                    static intptr_t transmitId = 0;
                                    canController->SendFrame(CanFrame{ std::move(data) }, reinterpret_cast<void*>(++transmitId));
                                    std::cout << "Demo >> SIL Kit : CAN frame (dlc=" << (int)data.dlc << " bytes, txId=" << transmitId << ")" << std::endl;
                                }};

        auto receiver_frameHandler = [&demoDevice](ICanController* /*controller*/, const CanFrameEvent& msg) 
        {
            std::cout << "SIL Kit >> Demo: CAN frame (" << msg.frame.dlc << " bytes)" << std::endl;
            std::vector<uint8_t> payloadBytes(msg.frame.dataField.data(), msg.frame.dataField.data() + msg.frame.dlc); 
            demoDevice.Process(std::move(msg.frame));
        };
    
        canController->AddFrameHandler(receiver_frameHandler);
        canController->AddFrameTransmitHandler(&CanAckCallback);
        canController->Start();

        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();

    }
    catch (const SilKit::ConfigurationError& error)
    {
        std::cerr << "Invalid configuration: " << error.what() << std::endl;
        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();
        return CONFIGURATION_ERROR;
    }
    catch (const InvalidCli&)
    {
        std::cout << "Usage (defaults in curly braces if you omit the switch):" << std::endl
                  << "SilKitDemoCanEchoDevice [" << participantNameArg  << " <participant's name{CanEchoDevice}>]\n"
                     "  ["  << regUriArg  << " silkit://<host{localhost}>:<port{8501}>]\n"
                     "  ["  << networkArg << " <SIL Kit CAN network name{CAN1}>]\n"
                     "  ["  << logLevelArg << " <Trace|Debug|Warn|{Info}|Error|Critical|off>]\n";

        std::cerr << std::endl << "Invalid command line arguments." << std::endl;
        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();
        return CLI_ERROR;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Something went wrong: " << error.what() << std::endl;
        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();
        return OTHER_ERROR;
    }
    return NO_ERROR;
}

