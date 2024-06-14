// Copyright (c) Vector Informatik GmbH. All rights reserved.
#include <iostream>
#include <unistd.h>

#include "Exceptions.hpp"
#include "Parsing.hpp"
#include "SignalHandler.hpp"
#include "AdapterConnections.hpp"
#include "SilKitAdapterSocketCAN.hpp"

#include "asio/ts/io_context.hpp"
#include "silkit/SilKit.hpp"
#include "silkit/config/all.hpp"
#include "silkit/services/can/all.hpp"

using namespace std;
using namespace SilKit::Services::Can;
using namespace exceptions;
using namespace adapters;
using namespace SilKit::Services::Orchestration;


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

        ICanController* canController = participant->CreateCanController(canControllerName, canNetworkName);

        auto can_connection = CanConnection::Create(io_context, canController, logger, canDevName.c_str());
        can_connection->ReceiveCanFrameFromVirtualCanDevice();

        SILKitInfoMessage.str("");
        SILKitInfoMessage << "Created CAN device connector for [" << canDevName << "] on network [" << canNetworkName << "]";
        logger->Info(SILKitInfoMessage.str());

        canController->Start();

        auto finalStateFuture = lifecycleService->StartLifecycle();

        thread t([&]() -> void {
            io_context.run();
        });

        PromptForExit();

        io_context.stop();
        t.join();
        canController->Stop();

        auto runningStateFuture = runningStatePromise.get_future();
        auto futureStatus = runningStateFuture.wait_for(15s);
        if (futureStatus != future_status::ready)
        {
            logger->Debug("Lifecycle Service Stopping: timed out while checking if the participant is currently running.");
        }

        lifecycleService->Stop("Adapter stopped by the user.");

        //canController->Start();
        auto finalState = finalStateFuture.wait_for(15s);
        if (finalState != future_status::ready)
        {
            logger->Debug("Lifecycle service stopping: timed out");
        }
    }
    catch (const SilKit::ConfigurationError& error)
    {
        cerr << "Invalid configuration: " << error.what() << endl;
        return CONFIGURATION_ERROR;
    }
    catch (const InvalidCli&)
    {
        print_help();
        cerr << endl << "Invalid command line arguments." << endl;
        return CLI_ERROR;
    }
    catch (const exception& error)
    {
        cerr << "Something went wrong: " << error.what() << endl;
        return OTHER_ERROR;
    }

    return NO_ERROR;
}
