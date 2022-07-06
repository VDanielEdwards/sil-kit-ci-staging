// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include "silkit/services/logging/ILogger.hpp"

#include "IServiceDiscovery.hpp"
#include "ServiceDatatypes.hpp"
#include "CanController.hpp"

namespace SilKit {
namespace Services {
namespace Can {

CanController::CanController(Core::IParticipantInternal* participant, SilKit::Config::CanController config,
                             Core::Orchestration::ITimeProvider* timeProvider)
    : _participant(participant)
    , _config{config}
    , _simulationBehavior{participant, this, timeProvider}
{
}

//------------------------
// Trivial or detailed
//------------------------

void CanController::RegisterServiceDiscovery()
{
    Core::Discovery::IServiceDiscovery* disc = _participant->GetServiceDiscovery();
    disc->RegisterServiceDiscoveryHandler([this](Core::Discovery::ServiceDiscoveryEvent::Type discoveryType,
                                                 const Core::ServiceDescriptor& remoteServiceDescriptor) {
        if (_simulationBehavior.IsTrivial())
        {
            // Check if received descriptor has a matching simulated link
            if (discoveryType == Core::Discovery::ServiceDiscoveryEvent::Type::ServiceCreated
                && IsRelevantNetwork(remoteServiceDescriptor))
            {
                SetDetailedBehavior(remoteServiceDescriptor);
            }
        }
        else
        {
            if (discoveryType == Core::Discovery::ServiceDiscoveryEvent::Type::ServiceRemoved
                && IsRelevantNetwork(remoteServiceDescriptor))
            {
                SetTrivialBehavior();
            }
        }
    });
}

void CanController::SetDetailedBehavior(const Core::ServiceDescriptor& remoteServiceDescriptor)
{
    _simulationBehavior.SetDetailedBehavior(remoteServiceDescriptor);
}
void CanController::SetTrivialBehavior()
{
    _simulationBehavior.SetTrivialBehavior();
}


auto CanController::GetState() -> CanControllerState
{
    return _controllerState;
}

auto CanController::IsRelevantNetwork(const Core::ServiceDescriptor& remoteServiceDescriptor) const -> bool
{
    // NetSim uses ServiceType::Link and the simulated networkName
    return remoteServiceDescriptor.GetServiceType() == SilKit::Core::ServiceType::Link
           && remoteServiceDescriptor.GetNetworkName() == _serviceDescriptor.GetNetworkName();
}

auto CanController::AllowReception(const IServiceEndpoint* from) const -> bool
{
    return _simulationBehavior.AllowReception(from);
}

template <typename MsgT>
void CanController::SendMsg(MsgT&& msg)
{
    _simulationBehavior.SendMsg(std::move(msg));
}

//------------------------
// Public API + Helpers
//------------------------

void CanController::SetBaudRate(uint32_t rate, uint32_t fdRate)
{
    _baudRate.baudRate = rate;
    _baudRate.fdBaudRate = fdRate;

    SendMsg(_baudRate);
}

void CanController::Reset()
{
    CanSetControllerMode mode;
    mode.flags.cancelTransmitRequests = 1;
    mode.flags.resetErrorHandling = 1;
    mode.mode = CanControllerState::Uninit;

    SendMsg(mode);
}

void CanController::Start()
{
    ChangeControllerMode(CanControllerState::Started);
}

void CanController::Stop()
{
    ChangeControllerMode(CanControllerState::Stopped);
}

void CanController::Sleep()
{
    ChangeControllerMode(CanControllerState::Sleep);
}

void CanController::ChangeControllerMode(CanControllerState state)
{
    CanSetControllerMode mode;
    mode.flags.cancelTransmitRequests = 0;
    mode.flags.resetErrorHandling = 0;
    mode.mode = state;

    SendMsg(mode);
}

auto CanController::SendFrame(const CanFrame& frame, void* userContext) -> CanTxId
{
    CanFrameEvent canFrameEvent{};
    canFrameEvent.frame = frame;
    canFrameEvent.transmitId = MakeTxId();
    canFrameEvent.userContext = userContext;

    SendMsg(canFrameEvent);
    return canFrameEvent.transmitId;
}

//------------------------
// ReceiveSilKitMessage
//------------------------

void CanController::ReceiveSilKitMessage(const IServiceEndpoint* from, const CanFrameEvent& msg)
{
    if (!AllowReception(from))
    {
        return;
    }

    _tracer.Trace(SilKit::Services::TransmitDirection::RX, msg.timestamp, msg);
    CallHandlers(msg);
}

void CanController::ReceiveSilKitMessage(const IServiceEndpoint* from, const CanFrameTransmitEvent& msg)
{
    if (!AllowReception(from))
    {
        return;
    }

    CallHandlers(msg);
}

void CanController::ReceiveSilKitMessage(const IServiceEndpoint* from, const CanControllerStatus& msg)
{
    if (!AllowReception(from))
    {
        return;
    }

    if (_controllerState != msg.controllerState)
    {
        _controllerState = msg.controllerState;
        CallHandlers(CanStateChangeEvent{ msg.timestamp, msg.controllerState });
    }
    if (_errorState != msg.errorState)
    {
        _errorState = msg.errorState;
        CallHandlers(CanErrorStateChangeEvent{ msg.timestamp, msg.errorState });
    }
}

//------------------------
// Handlers
//------------------------

HandlerId CanController::AddFrameHandler(FrameHandler handler, DirectionMask directionMask)
{
    auto filter = FilterT<CanFrameEvent>{[directionMask](const CanFrameEvent& frameEvent) {
        return (((DirectionMask)frameEvent.direction & (DirectionMask)directionMask)) != 0;
    }};
    return AddHandler(std::move(handler), std::move(filter));
}

void CanController::RemoveFrameHandler(HandlerId handlerId)
{
    if (!RemoveHandler<CanFrameEvent>(handlerId))
    {
        _participant->GetLogger()->Warn("RemoveFrameHandler failed: Unknown HandlerId.");
    }
}

HandlerId CanController::AddStateChangeHandler(StateChangeHandler handler)
{
    return AddHandler(std::move(handler));
}

void CanController::RemoveStateChangeHandler(HandlerId handlerId)
{
    if (!RemoveHandler<CanStateChangeEvent>(handlerId))
    {
        _participant->GetLogger()->Warn("RemoveStateChangeHandler failed: Unknown HandlerId.");
    }
}

HandlerId CanController::AddErrorStateChangeHandler(ErrorStateChangeHandler handler)
{
    return AddHandler(std::move(handler));
}

void CanController::RemoveErrorStateChangeHandler(HandlerId handlerId)
{
    if (!RemoveHandler<CanErrorStateChangeEvent>(handlerId))
    {
        _participant->GetLogger()->Warn("RemoveErrorStateChangeHandler failed: Unknown HandlerId.");
    }
}

HandlerId CanController::AddFrameTransmitHandler(FrameTransmitHandler handler, CanTransmitStatusMask statusMask)
{
    auto filter = FilterT<CanFrameTransmitEvent>{[statusMask](const CanFrameTransmitEvent& ack) {
        return ((CanTransmitStatusMask)ack.status & (CanTransmitStatusMask)statusMask) != 0;
    }};
    return AddHandler(std::move(handler), std::move(filter));
}

void CanController::RemoveFrameTransmitHandler(HandlerId handlerId)
{
    if (!RemoveHandler<CanFrameTransmitEvent>(handlerId))
    {
        _participant->GetLogger()->Warn("RemoveFrameTransmitHandler failed: Unknown HandlerId.");
    }
}

template <typename MsgT>
HandlerId CanController::AddHandler(CallbackT<MsgT> handler, FilterT<MsgT> filter)
{
    auto& callbacks = std::get<FilteredCallbacks<MsgT>>(_callbacks);
    return callbacks.Add(std::move(handler), std::move(filter));
}

template <typename MsgT>
bool CanController::RemoveHandler(HandlerId handlerId)
{
    auto& callbacks = std::get<FilteredCallbacks<MsgT>>(_callbacks);
    return callbacks.Remove(handlerId);
}

template <typename MsgT>
void CanController::CallHandlers(const MsgT& msg)
{
    auto& callbacks = std::get<FilteredCallbacks<MsgT>>(_callbacks);
    callbacks.InvokeAll(this, msg);
}

} // namespace Can
} // namespace Services
} // namespace SilKit
