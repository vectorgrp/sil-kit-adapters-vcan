// SPDX-FileCopyrightText: Copyright 2025 Vector Informatik GmbH
// SPDX-License-Identifier: MIT
#include <iostream>
#include <unistd.h>

#include "common/Parsing.hpp"

#include "AdapterConnections.hpp"
#include "SilKitAdapterSocketCAN.hpp"

#include "common/ParticipantCreation.hpp"
#include "common/Cli.hpp"

using namespace std;
using namespace SilKit::Services::Can;
using namespace adapters;
using namespace util;
using namespace SilKit::Services::Orchestration;

const std::string adapters::canNameArg = "--can-name";
const std::string adapters::networkArg = "--network";

void print_help(bool userRequested)
{
    std::cout << "Usage (defaults in curly braces if you omit the switch):" << std::endl;
    std::cout << "sil-kit-adapter-vcan [" << participantNameArg
              << " <participant name{SilKitAdapterVcan}>]\n"
                 "  ["
              << configurationArg
              << " <path to .silkit.yaml or .json configuration file>]\n"
                 "  ["
              << regUriArg
              << " silkit://<host{localhost}>:<port{8501}>]\n"
                 "  ["
              << logLevelArg
              << " <Trace|Debug|Warn|{Info}|Error|Critical|Off>]\n"
                 "  ["
              << canNameArg
              << " <vcan device name{can0}>]\n"
                 "  ["
              << networkArg
              << " <SIL Kit CAN network{CAN1}>]\n"
                 "SIL Kit-specific CLI arguments will be overwritten by the config file passed by "
              << configurationArg << ".\n";
    std::cout << "\n"
                 "Example:\n"
                 "sil-kit-adapter-vcan "
              << participantNameArg << " VCAN_PARTICIPANT " << networkArg << " CAN_NETWORK\n";

    if (!userRequested)
        std::cout << "\n"
                     "Pass "
                  << helpArg << " to get this message.\n";
};

int main(int argc, char** argv)
{
    if (findArg(argc, argv, helpArg, argv) != nullptr)
    {
        print_help(true);
        return CodeSuccess;
    }

    const std::string defaultParticipantName = "SilKitAdapterVcan";
    std::string participantName = defaultParticipantName;

    asio::io_context io_context;

    try
    {
        throwInvalidCliIf(thereAreUnknownArguments(
            argc, argv, {&networkArg, &canNameArg, &regUriArg, &logLevelArg, &participantNameArg, &configurationArg},
            {&helpArg}));

        const std::string canDevName = getArgDefault(argc, argv, canNameArg, "can0");
        const std::string canNetworkName = getArgDefault(argc, argv, networkArg, "CAN1");

        SilKit::Services::Logging::ILogger* logger;
        SilKit::Services::Orchestration::ILifecycleService* lifecycleService;
        std::promise<void> runningStatePromise;

        const auto participant =
            CreateParticipant(argc, argv, logger, &participantName, &lifecycleService, &runningStatePromise);

        const std::string canControllerName = "SilKit_CAN_CTRL_1";

        logger->Info("Creating CAN controller '" + canControllerName + "'");

        ICanController* canController = participant->CreateCanController(canControllerName, canNetworkName);

        auto can_connection = CanConnection::Create(io_context, canController, logger, canDevName.c_str());
        can_connection->ReceiveCanFrameFromVirtualCanDevice();

        logger->Info("Created CAN device connector for [" + canDevName + "]");

        canController->Start();

        auto finalStateFuture = lifecycleService->StartLifecycle();

        std::thread ioContextThread([&]() -> void { io_context.run(); });

        promptForExit();

        Stop(io_context, ioContextThread, *logger, &runningStatePromise, lifecycleService, &finalStateFuture);
    }
    catch (const SilKit::ConfigurationError& error)
    {
        cerr << "Invalid configuration: " << error.what() << endl;
        return CodeErrorConfiguration;
    }
    catch (const InvalidCli&)
    {
        print_help(false);
        cerr << endl << "Invalid command line arguments." << endl;
        return CodeErrorCli;
    }
    catch (const exception& error)
    {
        cerr << "Something went wrong: " << error.what() << endl;
        return CodeErrorOther;
    }

    return CodeSuccess;
}
