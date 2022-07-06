// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include "FlexrayController.hpp"
#include "Validation.hpp"
#include "IServiceDiscovery.hpp"
#include "ServiceDatatypes.hpp"

#include "silkit/services/logging/ILogger.hpp"

namespace SilKit {
namespace Services {
namespace Flexray {

FlexrayController::FlexrayController(Core::IParticipantInternal* participant, Config::FlexrayController config,
                                     Core::Orchestration::ITimeProvider* /*timeProvider*/)
    : _participant(participant)
    , _config{std::move(config)}
{
}

//------------------------
// Detailed Sim
//------------------------

void FlexrayController::RegisterServiceDiscovery()
{
    Core::Discovery::IServiceDiscovery* disc = _participant->GetServiceDiscovery();
    disc->RegisterServiceDiscoveryHandler(
        [this](Core::Discovery::ServiceDiscoveryEvent::Type discoveryType,
                                  const Core::ServiceDescriptor& remoteServiceDescriptor) {
            // check if discovered service is a network simulator (if none is known)
            if (!_simulatedLinkDetected)
            {
                // check if received descriptor has a matching simulated link
                if (discoveryType == Core::Discovery::ServiceDiscoveryEvent::Type::ServiceCreated
                    && IsRelevantNetwork(remoteServiceDescriptor))
                {
                    SetDetailedBehavior(remoteServiceDescriptor);
                }
            }
        });
}

auto FlexrayController::AllowReception(const IServiceEndpoint* from) const -> bool
{
    const auto& fromDescr = from->GetServiceDescriptor();
    return _simulatedLinkDetected &&
           _simulatedLink.GetParticipantName() == fromDescr.GetParticipantName()
           && _serviceDescriptor.GetServiceId() == fromDescr.GetServiceId();
}

auto FlexrayController::IsRelevantNetwork(const Core::ServiceDescriptor& remoteServiceDescriptor) const -> bool
{
    // NetSim uses ServiceType::Link and the simulated networkName
    return remoteServiceDescriptor.GetServiceType() == SilKit::Core::ServiceType::Link
           && remoteServiceDescriptor.GetNetworkName() == _serviceDescriptor.GetNetworkName();
}

// Expose for testing purposes
void FlexrayController::SetDetailedBehavior(const Core::ServiceDescriptor& remoteServiceDescriptor)
{
    _simulatedLinkDetected = true;
    _simulatedLink = remoteServiceDescriptor;
}

//------------------------
// Public API + Helpers
//------------------------

bool FlexrayController::IsClusterParametersConfigurable()
{
    return !_config.clusterParameters.has_value();
}

bool FlexrayController::IsNodeParametersConfigurable()
{
    return !_config.nodeParameters.has_value();
}

bool FlexrayController::IsTxBufferConfigsConfigurable()
{
    return _config.txBufferConfigurations.empty();
}

void FlexrayController::WarnOverride(const std::string& parameterName)
{
    std::stringstream ss;
    ss << "Discarded user-defined configuration of " << parameterName
       << ", as it was already set in the predefined configuration.";

    _participant->GetLogger()->Warn(ss.str());
}

void FlexrayController::Configure(const FlexrayControllerConfig& config)
{
    FlexrayControllerConfig cfg = config;
    if (!IsClusterParametersConfigurable())
    {
        cfg.clusterParams = _config.clusterParameters.value();
        WarnOverride("clusterParameters");
    }
    if (!IsNodeParametersConfigurable())
    {
        cfg.nodeParams = _config.nodeParameters.value();
        WarnOverride("NodeParamters");
    }
    if (!IsTxBufferConfigsConfigurable())
    {
        cfg.bufferConfigs = _config.txBufferConfigurations;
        WarnOverride("TxBufferConfigs");
    }

    Validate(cfg.clusterParams);
    Validate(cfg.nodeParams);

    _bufferConfigs = cfg.bufferConfigs;
    SendMsg(cfg);
}

void FlexrayController::ReconfigureTxBuffer(uint16_t txBufferIdx, const FlexrayTxBufferConfig& config)
{
    if (txBufferIdx >= _bufferConfigs.size())
    {
        _participant->GetLogger()->Error("FlexrayController::ReconfigureTxBuffer() was called with unconfigured txBufferIdx={}", txBufferIdx);
        throw std::out_of_range{"Unconfigured txBufferIdx!"};
    }

    if (!IsTxBufferConfigsConfigurable())
    {
        _participant->GetLogger()->Error("ReconfigureTxBuffer() was called on a preconfigured txBuffer. This is not "
                                        "allowed and the reconfiguration will be discarded.");
        return;
    }

    FlexrayTxBufferConfigUpdate update;
    update.txBufferIndex = txBufferIdx;
    update.txBufferConfig = config;
    SendMsg(update);
}

void FlexrayController::UpdateTxBuffer(const FlexrayTxBufferUpdate& update)
{
    if (update.txBufferIndex >= _bufferConfigs.size())
    {
        _participant->GetLogger()->Error("FlexrayController::UpdateTxBuffer() was called with unconfigured txBufferIndex={}", update.txBufferIndex);
        throw std::out_of_range{"Unconfigured txBufferIndex!"};
    }

    SendMsg(update);
}

void FlexrayController::Run()
{
    FlexrayHostCommand cmd;
    cmd.command = FlexrayChiCommand::RUN;
    SendMsg(cmd);
}

void FlexrayController::DeferredHalt()
{
    FlexrayHostCommand cmd;
    cmd.command = FlexrayChiCommand::DEFERRED_HALT;
    SendMsg(cmd);
}

void FlexrayController::Freeze()
{
    FlexrayHostCommand cmd;
    cmd.command = FlexrayChiCommand::FREEZE;
    SendMsg(cmd);
}

void FlexrayController::AllowColdstart()
{
    FlexrayHostCommand cmd;
    cmd.command = FlexrayChiCommand::ALLOW_COLDSTART;
    SendMsg(cmd);
}

void FlexrayController::AllSlots()
{
    FlexrayHostCommand cmd;
    cmd.command = FlexrayChiCommand::ALL_SLOTS;
    SendMsg(cmd);
}

void FlexrayController::Wakeup()
{
    FlexrayHostCommand cmd;
    cmd.command = FlexrayChiCommand::WAKEUP;
    SendMsg(cmd);
}

//------------------------
// ReceiveSilKitMessage
//------------------------

void FlexrayController::ReceiveSilKitMessage(const IServiceEndpoint* from, const FlexrayFrameEvent& msg)
{
    if (!AllowReception(from))
    {
        return;
    }

    _tracer.Trace(SilKit::Services::TransmitDirection::RX, msg.timestamp, msg);
    CallHandlers(msg);
}

void FlexrayController::ReceiveSilKitMessage(const IServiceEndpoint* from, const FlexrayFrameTransmitEvent& msg)
{
    if (!AllowReception(from))
    {
        return;
    }

    FlexrayFrameEvent tmp;
    tmp.frame = msg.frame;
    tmp.channel = msg.channel;
    tmp.timestamp = msg.timestamp;
    _tracer.Trace(SilKit::Services::TransmitDirection::TX, msg.timestamp, tmp);

    CallHandlers(msg);
}

void FlexrayController::ReceiveSilKitMessage(const IServiceEndpoint* from, const FlexraySymbolEvent& msg)
{
    if (!AllowReception(from))
    {
        return;
    }

    // Call wakeup handlers on WUS and WUDOP
    switch (msg.pattern)
    {
    case FlexraySymbolPattern::CasMts:
        break;
    case FlexraySymbolPattern::Wus:
    case FlexraySymbolPattern::Wudop:
        // Synthesize a FlexrayWakeupEvent triggered by this FlexraySymbolEvent
        CallHandlers(FlexrayWakeupEvent{msg});
    }

    // In addition, call the generic SymbolHandlers for every symbol
    CallHandlers(msg);
}

void FlexrayController::ReceiveSilKitMessage(const IServiceEndpoint* from, const FlexraySymbolTransmitEvent& msg)
{
    if (!AllowReception(from))
    {
        return;
    }

    CallHandlers(msg);
}

void FlexrayController::ReceiveSilKitMessage(const IServiceEndpoint* from, const FlexrayCycleStartEvent& msg)
{
    if (!AllowReception(from))
    {
        return;
    }

    CallHandlers(msg);
}

void FlexrayController::ReceiveSilKitMessage(const IServiceEndpoint* from, const FlexrayPocStatusEvent& msg)
{
    if (!AllowReception(from))
    {
        return;
    }

    CallHandlers(msg);
}


template <typename MsgT>
void FlexrayController::SendMsg(MsgT&& msg)
{
    _participant->SendMsg(this, std::forward<MsgT>(msg));
}

//------------------------
// Handlers
//------------------------

HandlerId FlexrayController::AddFrameHandler(FrameHandler handler)
{
    return AddHandler(std::move(handler));
}

void FlexrayController::RemoveFrameHandler(HandlerId handlerId)
{
    if (!RemoveHandler<FlexrayFrameEvent>(handlerId))
    {
        _participant->GetLogger()->Warn("RemoveFrameHandler failed: Unknown HandlerId.");
    }
}

HandlerId FlexrayController::AddFrameTransmitHandler(FrameTransmitHandler handler)
{
    return AddHandler(std::move(handler));
}

void FlexrayController::RemoveFrameTransmitHandler(HandlerId handlerId)
{
    if (!RemoveHandler<FlexrayFrameTransmitEvent>(handlerId))
    {
        _participant->GetLogger()->Warn("RemoveFrameTransmitHandler failed: Unknown HandlerId.");
    }
}

HandlerId FlexrayController::AddWakeupHandler(WakeupHandler handler)
{
    return AddHandler(std::move(handler));
}

void FlexrayController::RemoveWakeupHandler(HandlerId handlerId)
{
    if (!RemoveHandler<FlexrayWakeupEvent>(handlerId))
    {
        _participant->GetLogger()->Warn("RemoveWakeupHandler failed: Unknown HandlerId.");
    }
}

HandlerId FlexrayController::AddPocStatusHandler(PocStatusHandler handler)
{
    return AddHandler(std::move(handler));
}

void FlexrayController::RemovePocStatusHandler(HandlerId handlerId)
{
    if (!RemoveHandler<FlexrayPocStatusEvent>(handlerId))
    {
        _participant->GetLogger()->Warn("RemovePocStatusHandler failed: Unknown HandlerId.");
    }
}

HandlerId FlexrayController::AddSymbolHandler(SymbolHandler handler)
{
    return AddHandler(std::move(handler));
}

void FlexrayController::RemoveSymbolHandler(HandlerId handlerId)
{
    if (!RemoveHandler<FlexraySymbolEvent>(handlerId))
    {
        _participant->GetLogger()->Warn("RemoveSymbolHandler failed: Unknown HandlerId.");
    }
}

HandlerId FlexrayController::AddSymbolTransmitHandler(SymbolTransmitHandler handler)
{
    return AddHandler(std::move(handler));
}

void FlexrayController::RemoveSymbolTransmitHandler(HandlerId handlerId)
{
    if (!RemoveHandler<FlexraySymbolTransmitEvent>(handlerId))
    {
        _participant->GetLogger()->Warn("RemoveSymbolTransmitHandler failed: Unknown HandlerId.");
    }
}

HandlerId FlexrayController::AddCycleStartHandler(CycleStartHandler handler)
{
    return AddHandler(std::move(handler));
}

void FlexrayController::RemoveCycleStartHandler(HandlerId handlerId)
{
    if (!RemoveHandler<FlexrayCycleStartEvent>(handlerId))
    {
        _participant->GetLogger()->Warn("RemoveCycleStartHandler failed: Unknown HandlerId.");
    }
}

template <typename MsgT>
HandlerId FlexrayController::AddHandler(CallbackT<MsgT> handler)
{
    auto& callbacks = std::get<CallbacksT<MsgT>>(_callbacks);
    return callbacks.Add(std::move(handler));
}

template <typename MsgT>
auto FlexrayController::RemoveHandler(HandlerId handlerId) -> bool
{
    auto& callbacks = std::get<CallbacksT<MsgT>>(_callbacks);
    return callbacks.Remove(handlerId);
}

template <typename MsgT>
void FlexrayController::CallHandlers(const MsgT& msg)
{
    auto& callbacks = std::get<CallbacksT<MsgT>>(_callbacks);
    callbacks.InvokeAll(this, msg);
}

} // namespace Flexray
} // namespace Services
} // namespace SilKit