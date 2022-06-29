// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include "FlexraySerdes.hpp"

namespace ib {
namespace sim {
namespace fr {
using ib::mw::MessageBuffer;

ib::mw::MessageBuffer& operator<<(ib::mw::MessageBuffer& buffer, const FlexrayHeader& header)
{
    buffer
        << header.flags
        << header.frameId
        << header.payloadLength
        << header.headerCrc
        << header.cycleCount;
    return buffer;
}

ib::mw::MessageBuffer& operator>>(ib::mw::MessageBuffer& buffer, FlexrayHeader& header)
{
    buffer
        >> header.flags
        >> header.frameId
        >> header.payloadLength
        >> header.headerCrc
        >> header.cycleCount;
    return buffer;
}

ib::mw::MessageBuffer& operator<<(ib::mw::MessageBuffer& buffer, const FlexrayFrame& frame)
{
    buffer
        << frame.header
        << frame.payload;
    return buffer;
}

ib::mw::MessageBuffer& operator>>(ib::mw::MessageBuffer& buffer, FlexrayFrame& frame)
{
    buffer
        >> frame.header
        >> frame.payload;
    return buffer;
}

ib::mw::MessageBuffer& operator<<(ib::mw::MessageBuffer& buffer, const FlexrayFrameEvent& msg)
{
    buffer
        << msg.timestamp
        << msg.channel
        << msg.frame;
    return buffer;
}

ib::mw::MessageBuffer& operator>>(ib::mw::MessageBuffer& buffer, FlexrayFrameEvent& msg)
{
    buffer
        >> msg.timestamp
        >> msg.channel
        >> msg.frame;
    return buffer;
}

ib::mw::MessageBuffer& operator<<(ib::mw::MessageBuffer& buffer, const FlexrayFrameTransmitEvent& msg)
{
    buffer
        << msg.timestamp
        << msg.txBufferIndex
        << msg.channel
        << msg.frame;
    return buffer;
}

ib::mw::MessageBuffer& operator>>(ib::mw::MessageBuffer& buffer, FlexrayFrameTransmitEvent& msg)
{
    buffer
        >> msg.timestamp
        >> msg.txBufferIndex
        >> msg.channel
        >> msg.frame;
    return buffer;
}

ib::mw::MessageBuffer& operator<<(ib::mw::MessageBuffer& buffer, const FlexraySymbolEvent& symbol)
{
    buffer
        << symbol.timestamp
        << symbol.channel
        << symbol.pattern;
    return buffer;
}

ib::mw::MessageBuffer& operator>>(ib::mw::MessageBuffer& buffer, FlexraySymbolEvent& symbol)
{
    buffer
        >> symbol.timestamp
        >> symbol.channel
        >> symbol.pattern;
    return buffer;
}

ib::mw::MessageBuffer& operator<<(ib::mw::MessageBuffer& buffer, const FlexraySymbolTransmitEvent& ack)
{
    auto&& symbol = static_cast<const FlexraySymbolEvent&>(ack);
    buffer
        << symbol;
    return buffer;
}

ib::mw::MessageBuffer& operator>>(ib::mw::MessageBuffer& buffer, FlexraySymbolTransmitEvent& ack)
{
    auto&& symbol = static_cast<FlexraySymbolEvent&>(ack);
    buffer
        >> symbol;
    return buffer;
}

ib::mw::MessageBuffer& operator<<(ib::mw::MessageBuffer& buffer,
                                         const FlexrayCycleStartEvent& flexrayCycleStartEvent)
{
    buffer
        << flexrayCycleStartEvent.timestamp
        << flexrayCycleStartEvent.cycleCounter;
    return buffer;
}

ib::mw::MessageBuffer& operator>>(ib::mw::MessageBuffer& buffer, FlexrayCycleStartEvent& flexrayCycleStartEvent)
{
    buffer
        >> flexrayCycleStartEvent.timestamp
        >> flexrayCycleStartEvent.cycleCounter;
    return buffer;
}

ib::mw::MessageBuffer& operator<<(ib::mw::MessageBuffer& buffer, const FlexrayHostCommand& cmd)
{
    buffer
        << cmd.command;
    return buffer;
}

ib::mw::MessageBuffer& operator>>(ib::mw::MessageBuffer& buffer, FlexrayHostCommand& cmd)
{
    buffer
        >> cmd.command;
    return buffer;
}

ib::mw::MessageBuffer& operator<<(ib::mw::MessageBuffer& buffer, const FlexrayClusterParameters& clusterParam)
{
    buffer
        << clusterParam.gColdstartAttempts
        << clusterParam.gCycleCountMax
        << clusterParam.gdActionPointOffset
        << clusterParam.gdDynamicSlotIdlePhase
        << clusterParam.gdMiniSlot
        << clusterParam.gdMiniSlotActionPointOffset
        << clusterParam.gdStaticSlot
        << clusterParam.gdSymbolWindow
        << clusterParam.gdSymbolWindowActionPointOffset
        << clusterParam.gdTSSTransmitter
        << clusterParam.gdWakeupTxActive
        << clusterParam.gdWakeupTxIdle
        << clusterParam.gListenNoise
        << clusterParam.gMacroPerCycle
        << clusterParam.gMaxWithoutClockCorrectionFatal
        << clusterParam.gMaxWithoutClockCorrectionPassive
        << clusterParam.gNumberOfMiniSlots
        << clusterParam.gNumberOfStaticSlots
        << clusterParam.gPayloadLengthStatic
        << clusterParam.gSyncFrameIDCountMax;
    return buffer;
}

ib::mw::MessageBuffer& operator>>(ib::mw::MessageBuffer& buffer, FlexrayClusterParameters& clusterParam)
{
    buffer
        >> clusterParam.gColdstartAttempts
        >> clusterParam.gCycleCountMax
        >> clusterParam.gdActionPointOffset
        >> clusterParam.gdDynamicSlotIdlePhase
        >> clusterParam.gdMiniSlot
        >> clusterParam.gdMiniSlotActionPointOffset
        >> clusterParam.gdStaticSlot
        >> clusterParam.gdSymbolWindow
        >> clusterParam.gdSymbolWindowActionPointOffset
        >> clusterParam.gdTSSTransmitter
        >> clusterParam.gdWakeupTxActive
        >> clusterParam.gdWakeupTxIdle
        >> clusterParam.gListenNoise
        >> clusterParam.gMacroPerCycle
        >> clusterParam.gMaxWithoutClockCorrectionFatal
        >> clusterParam.gMaxWithoutClockCorrectionPassive
        >> clusterParam.gNumberOfMiniSlots
        >> clusterParam.gNumberOfStaticSlots
        >> clusterParam.gPayloadLengthStatic
        >> clusterParam.gSyncFrameIDCountMax;
    return buffer;
}

ib::mw::MessageBuffer& operator<<(ib::mw::MessageBuffer& buffer, const FlexrayNodeParameters& nodeParams)
{
    buffer
        << nodeParams.pAllowHaltDueToClock
        << nodeParams.pAllowPassiveToActive
        << nodeParams.pChannels
        << nodeParams.pClusterDriftDamping
        << nodeParams.pdAcceptedStartupRange
        << nodeParams.pdListenTimeout
        << nodeParams.pKeySlotId
        << nodeParams.pKeySlotOnlyEnabled
        << nodeParams.pKeySlotUsedForStartup
        << nodeParams.pKeySlotUsedForSync
        << nodeParams.pLatestTx
        << nodeParams.pMacroInitialOffsetA
        << nodeParams.pMacroInitialOffsetB
        << nodeParams.pMicroInitialOffsetA
        << nodeParams.pMicroInitialOffsetB
        << nodeParams.pMicroPerCycle
        << nodeParams.pOffsetCorrectionOut
        << nodeParams.pOffsetCorrectionStart
        << nodeParams.pRateCorrectionOut
        << nodeParams.pWakeupChannel
        << nodeParams.pWakeupPattern
        << nodeParams.pdMicrotick
        << nodeParams.pSamplesPerMicrotick;
    return buffer;
}

ib::mw::MessageBuffer& operator>>(ib::mw::MessageBuffer& buffer, FlexrayNodeParameters& nodeParams)
{
    buffer
        >> nodeParams.pAllowHaltDueToClock
        >> nodeParams.pAllowPassiveToActive
        >> nodeParams.pChannels
        >> nodeParams.pClusterDriftDamping
        >> nodeParams.pdAcceptedStartupRange
        >> nodeParams.pdListenTimeout
        >> nodeParams.pKeySlotId
        >> nodeParams.pKeySlotOnlyEnabled
        >> nodeParams.pKeySlotUsedForStartup
        >> nodeParams.pKeySlotUsedForSync
        >> nodeParams.pLatestTx
        >> nodeParams.pMacroInitialOffsetA
        >> nodeParams.pMacroInitialOffsetB
        >> nodeParams.pMicroInitialOffsetA
        >> nodeParams.pMicroInitialOffsetB
        >> nodeParams.pMicroPerCycle
        >> nodeParams.pOffsetCorrectionOut
        >> nodeParams.pOffsetCorrectionStart
        >> nodeParams.pRateCorrectionOut
        >> nodeParams.pWakeupChannel
        >> nodeParams.pWakeupPattern
        >> nodeParams.pdMicrotick
        >> nodeParams.pSamplesPerMicrotick;
    return buffer;
}

ib::mw::MessageBuffer& operator<<(ib::mw::MessageBuffer& buffer, const FlexrayTxBufferConfig& config)
{
    buffer
        << config.channels
        << config.slotId
        << config.offset
        << config.repetition
        << config.hasPayloadPreambleIndicator
        << config.headerCrc
        << config.transmissionMode;
    return buffer;
}

ib::mw::MessageBuffer& operator>>(ib::mw::MessageBuffer& buffer, FlexrayTxBufferConfig& config)
{
    buffer
        >> config.channels
        >> config.slotId
        >> config.offset
        >> config.repetition
        >> config.hasPayloadPreambleIndicator
        >> config.headerCrc
        >> config.transmissionMode;
    return buffer;
}

ib::mw::MessageBuffer& operator<<(ib::mw::MessageBuffer& buffer, const FlexrayControllerConfig& config)
{
    buffer
        << config.clusterParams
        << config.nodeParams
        << config.bufferConfigs;
    return buffer;
}

ib::mw::MessageBuffer& operator>>(ib::mw::MessageBuffer& buffer, FlexrayControllerConfig& config)
{
    buffer
        >> config.clusterParams
        >> config.nodeParams
        >> config.bufferConfigs;
    return buffer;
}

ib::mw::MessageBuffer& operator<<(ib::mw::MessageBuffer& buffer, const FlexrayTxBufferConfigUpdate& update)
{
    buffer
        << update.txBufferIndex
        << update.txBufferConfig;
    return buffer;
}

ib::mw::MessageBuffer& operator>>(ib::mw::MessageBuffer& buffer, FlexrayTxBufferConfigUpdate& update)
{
    buffer
        >> update.txBufferIndex
        >> update.txBufferConfig;
    return buffer;
}

ib::mw::MessageBuffer& operator<<(ib::mw::MessageBuffer& buffer, const FlexrayTxBufferUpdate& update)
{
    buffer
        << update.txBufferIndex
        << update.payloadDataValid
        << update.payload;
    return buffer;
}

ib::mw::MessageBuffer& operator>>(ib::mw::MessageBuffer& buffer, FlexrayTxBufferUpdate& update)
{
    buffer
        >> update.txBufferIndex
        >> update.payloadDataValid
        >> update.payload;
    return buffer;
}

ib::mw::MessageBuffer& operator<<(ib::mw::MessageBuffer& buffer,
                                         const FlexrayPocStatusEvent& flexrayPocStatusEvent)
{
    buffer
        << flexrayPocStatusEvent.timestamp
        << flexrayPocStatusEvent.chiHaltRequest
        << flexrayPocStatusEvent.coldstartNoise
        << flexrayPocStatusEvent.errorMode
        << flexrayPocStatusEvent.freeze
        << flexrayPocStatusEvent.slotMode
        << flexrayPocStatusEvent.startupState
        << flexrayPocStatusEvent.state
        << flexrayPocStatusEvent.wakeupStatus
        << flexrayPocStatusEvent.chiReadyRequest;
    return buffer;
}

ib::mw::MessageBuffer& operator>>(ib::mw::MessageBuffer& buffer, FlexrayPocStatusEvent& flexrayPocStatusEvent)
{
    buffer
      >> flexrayPocStatusEvent.timestamp
      >> flexrayPocStatusEvent.chiHaltRequest
      >> flexrayPocStatusEvent.coldstartNoise
      >> flexrayPocStatusEvent.errorMode
      >> flexrayPocStatusEvent.freeze
      >> flexrayPocStatusEvent.slotMode
      >> flexrayPocStatusEvent.startupState
      >> flexrayPocStatusEvent.state
      >> flexrayPocStatusEvent.wakeupStatus
      >> flexrayPocStatusEvent.chiReadyRequest;
    return buffer;
}


void Serialize(MessageBuffer& buffer, const FlexrayFrameEvent& msg)
{
    buffer << msg;
    return;
}

void Serialize(MessageBuffer& buffer, const FlexrayFrameTransmitEvent& msg)
{
    buffer << msg;
    return;
}

void Serialize(MessageBuffer& buffer, const FlexraySymbolEvent& msg)
{
    buffer << msg;
    return;
}

void Serialize(MessageBuffer& buffer, const FlexraySymbolTransmitEvent& msg)
{
    buffer << msg;
    return;
}

void Serialize(MessageBuffer& buffer, const FlexrayCycleStartEvent& msg)
{
    buffer << msg;
    return;
}

void Serialize(MessageBuffer& buffer, const FlexrayHostCommand& msg)
{
    buffer << msg;
    return;
}

void Serialize(MessageBuffer& buffer, const FlexrayControllerConfig& msg)
{
    buffer << msg;
    return;
}

void Serialize(MessageBuffer& buffer, const FlexrayTxBufferConfigUpdate& msg)
{
    buffer << msg;
    return;
}

void Serialize(MessageBuffer& buffer, const FlexrayTxBufferUpdate& msg)
{
    buffer << msg;
    return;
}

void Serialize(MessageBuffer& buffer, const FlexrayPocStatusEvent& msg)
{
    buffer << msg;
    return;
}

void Deserialize(MessageBuffer& buffer, FlexrayFrameEvent& out)
{
    buffer >> out;
}

void Deserialize(MessageBuffer& buffer, FlexrayFrameTransmitEvent& out)
{
    buffer >> out;
}

void Deserialize(MessageBuffer& buffer, FlexraySymbolEvent& out)
{
    buffer >> out;
}

void Deserialize(MessageBuffer& buffer, FlexraySymbolTransmitEvent& out)
{
    buffer >> out;
}

void Deserialize(MessageBuffer& buffer, FlexrayCycleStartEvent& out)
{
    buffer >> out;
}

void Deserialize(MessageBuffer& buffer, FlexrayHostCommand& out)
{
    buffer >> out;
}

void Deserialize(MessageBuffer& buffer, FlexrayControllerConfig& out)
{
    buffer >> out;
}

void Deserialize(MessageBuffer& buffer, FlexrayTxBufferConfigUpdate& out)
{
    buffer >> out;
}

void Deserialize(MessageBuffer& buffer, FlexrayTxBufferUpdate& out)
{
    buffer >> out;
}

void Deserialize(MessageBuffer& buffer, FlexrayPocStatusEvent& out)
{
    buffer >> out;
}

} // namespace fr
} // namespace sim
} // namespace ib
