// Copyright (c) Vector Informatik GmbH. All rights reserved.
#include "YamlConfig.hpp"

#include <algorithm>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <chrono>

#include "ib/cfg/OptionalCfg.hpp"
#include "ib/cfg/string_utils.hpp"

//local utilities
namespace {
using namespace ib::cfg;

inline auto nibble_to_char(char nibble) -> char
{
    nibble &= 0xf;
    if (0 <= nibble && nibble < 10)
        return '0' + nibble;
    else
        return 'a' + (nibble - 10);
}

inline auto char_to_nibble(char c) -> char
{
    if (c < '0' || c > 'f')
        throw std::runtime_error("OutOfRange");


    if (c < 'a')
        return c - '0';
    else
        return c - 'a' + 10;
}

auto hex_encode(const std::vector<uint8_t>& data) -> std::string
{
    std::stringstream out;
    for (auto&& byte : data)
    {
        out << nibble_to_char(byte >> 4)
            << nibble_to_char(byte);
    }
    return out.str();
}

auto hex_decode(const std::string& str) -> std::vector<uint8_t>
{
    if (str.size() % 2 != 0)
        throw std::runtime_error("InvalidStrFormat");

    std::vector<uint8_t> result;
    result.reserve(str.size() / 2);

    for (auto iter = str.begin(); iter != str.end(); iter += 2)
    {
        char high = char_to_nibble(*iter);
        char low = char_to_nibble(*(iter + 1));

        result.push_back(high << 4 | low);
    }
    return result;
}

auto macaddress_encode(const std::array<uint8_t, 6>& macAddress) -> YAML::Node
{
    std::stringstream macOut;

    to_ostream(macOut, macAddress);

    return YAML::Node(macOut.str());
}

auto macaddress_decode(const YAML::Node& node) -> std::array<uint8_t, 6>
{
    std::array<uint8_t, 6> macAddress;

    std::stringstream macIn(node.as<std::string>());
    from_istream(macIn, macAddress);

    return macAddress;
}

template<typename ConfigT>
void optional_encode(const OptionalCfg<ConfigT>& value, YAML::Node& node, const std::string& fieldName)
{
    if (value.has_value())
    {
        node[fieldName] = value.value();
    }
}
template<typename ConfigT>
void optional_encode(const std::vector<ConfigT>& value, YAML::Node& node, const std::string& fieldName)
{
    if (value.size() > 0)
    {
        node[fieldName] = value;
    }
}

void optional_encode(const Replay& value, YAML::Node& node, const std::string& fieldName)
{
    if (value.useTraceSource.size() > 0)
    {
        node[fieldName] = value;
    }
}

template<typename ConfigT>
void optional_decode(OptionalCfg<ConfigT>& value, const YAML::Node& node, const std::string& fieldName)
{
    if (node[fieldName]) //operator[] does not modify node
    {
        value = node[fieldName].as<ConfigT>();
    }
}

template<typename ConfigT>
void optional_decode(ConfigT& value, const YAML::Node& node, const std::string& fieldName)
{
    if (node[fieldName]) //operator[] does not modify node
    {
        value = node[fieldName].as<ConfigT>();
    }
}

template <typename ConfigT>
auto non_default_encode(const std::vector<ConfigT>& values, YAML::Node& node, const std::string& fieldName, const ConfigT& defaultValue)
{
    if (!(values == defaultValue))
    {
        auto&& sequence = node[fieldName];
        std::copy(values.begin(), values.end(), sequence.begin());
    }
}

template <typename ConfigT>
auto non_default_encode(const ConfigT& value, YAML::Node& node, const std::string& fieldName, const ConfigT& defaultValue)
{
    if (!(value == defaultValue))
        node[fieldName] = value;
}

} //end anonymous

// YAML type conversion helpers for our data types
namespace YAML {

using namespace ib;

template<>
Node VibConversion::encode(const MdfChannel& obj)
{
    Node node;
    optional_encode(obj.channelName, node, "ChannelName");
    optional_encode(obj.channelPath, node, "ChannelPath");
    optional_encode(obj.channelSource, node, "ChannelSource");
    optional_encode(obj.groupName, node, "GroupName");
    optional_encode(obj.groupPath, node, "GroupPath");
    optional_encode(obj.groupSource, node, "GroupSource");
    return node;
}
template<>
bool VibConversion::decode(const Node& node, MdfChannel& obj)
{
    if (!node.IsMap())
    {
        return false;
    }
    optional_decode(obj.channelName, node, "ChannelName");
    optional_decode(obj.channelPath, node, "ChannelPath");
    optional_decode(obj.channelSource, node, "ChannelSource");
    optional_decode(obj.groupName, node, "GroupName");
    optional_decode(obj.groupPath, node, "GroupPath");
    optional_decode(obj.groupSource, node, "GroupSource");
    return true;
}

// copy paste
template<>
Node VibConversion::encode(const Version& obj)
{
    Node node;
    std::stringstream ss;
    ss << obj;
    node = ss.str();
    return node;
}
template<>
bool VibConversion::decode(const Node& node, Version& obj)
{
    if (!node.IsScalar())
    {
        return false;
    }
    std::stringstream in(node.as<std::string>());
    in >> obj;
    return !in.fail();
}

template<>
Node VibConversion::encode(const Sink::Type& obj)
{
    Node node;
    switch (obj)
    {
    case Sink::Type::Remote:
        node = "Remote";
        break;
    case Sink::Type::Stdout:
        node = "Stdout";
        break;
    case Sink::Type::File:
        node = "File";
        break;
    default:
        break; 
    }
    return node;
}
template<>
bool VibConversion::decode(const Node& node, Sink::Type& obj)
{
    if (!node.IsScalar())
    {
        return false;
    }
    auto&& str = node.as<std::string>();
    if (str == "Remote" || str == "")
    {
        obj = Sink::Type::Remote;
    }
    else if (str == "Stdout")
    {
        obj = Sink::Type::Stdout;
    }
    else if (str == "File")
    {
        obj = Sink::Type::File;
    }
    else 
    {
        return false;
    }
    return true;
}

template<>
Node VibConversion::encode(const mw::logging::Level& obj)
{
    Node node;
    switch (obj)
    {
    case mw::logging::Level::Critical:
        node = "Critical";
        break;
    case mw::logging::Level::Error:
        node = "Error";
        break;
    case mw::logging::Level::Warn:
        node = "Warn";
        break;
    case mw::logging::Level::Info:
        node = "Info";
        break;
    case mw::logging::Level::Debug:
        node = "Debug";
        break;
    case mw::logging::Level::Trace:
        node = "Trace";
        break;
    case mw::logging::Level::Off:
        node =  "Off";
        break;
    }
    return node;
}
template<>
bool VibConversion::decode(const Node& node, mw::logging::Level& obj)
{
    auto&& str = node.as<std::string>();
    if (str == "Critical")
        obj = mw::logging::Level::Critical;
    else if (str == "Error")
        obj = mw::logging::Level::Error;
    else if (str == "Warn")
        obj = mw::logging::Level::Warn;
    else if (str == "Info")
        obj = mw::logging::Level::Info;
    else if (str == "Debug")
        obj = mw::logging::Level::Debug;
    else if (str == "Trace")
        obj = mw::logging::Level::Trace;
    else if (str == "Off")
        obj = mw::logging::Level::Off;
    else
        return false;
    return true;
}

template<>
Node VibConversion::encode(const Sink& obj)
{
    static const Sink defaultSink{};
    Node node;
    non_default_encode(obj.type, node, "Type", defaultSink.type);
    non_default_encode(obj.level, node, "Level", defaultSink.level);
    non_default_encode(obj.logname, node, "Logname", defaultSink.logname);
    return node;
}
template<>
bool VibConversion::decode(const Node& node, Sink& obj)
{
    obj.type = node["Type"].as<Sink::Type>(); 
    optional_decode(obj.level, node, "Level");

    if (obj.type == Sink::Type::File)
    {
        if (!node["Logname"])
        {
            return false; //XXX throw here with message?
        }
        obj.logname = node["Logname"].as<std::string>();
    }

    return true;
}

template<>
Node VibConversion::encode(const Logger& obj)
{
    Node node;
    static const Logger defaultLogger{};

    non_default_encode(obj.logFromRemotes, node, "LogFromRemotes", defaultLogger.logFromRemotes);
    non_default_encode(obj.flush_level, node, "FlushLevel", defaultLogger.flush_level);
    non_default_encode(obj.sinks, node, "Sinks", defaultLogger.sinks);

    return node;
}
template<>
bool VibConversion::decode(const Node& node, Logger& obj)
{
    optional_decode(obj.logFromRemotes, node, "LogFromRemotes");
    optional_decode(obj.flush_level, node, "FlushLevel");
    optional_decode(obj.sinks, node, "Sinks");
    return true;
}

template<>
Node VibConversion::encode(const CanController& obj)
{
    Node node;
    node["Name"] = obj.name;
    optional_encode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_encode(obj.replay, node, "Replay");
    return node;
}
template<>
bool VibConversion::decode(const Node& node, CanController& obj)
{
    // backward compatibility to old json config files with only name of controller
    if (node.IsScalar())
    {
        obj.name = node.as<std::string>();
        return true;
    }

    obj.name = node["Name"].as<std::string>();
    if (node["UseTraceSinks"])
    {
        obj.useTraceSinks = node["UseTraceSinks"].as<decltype(obj.useTraceSinks)>();
    }
    optional_decode(obj.replay, node, "Replay");
    return true;
}

template<>
Node VibConversion::encode(const LinController& obj)
{
    Node node;
    node["Name"] = obj.name;

    optional_encode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_encode(obj.replay, node, "Replay");
    return node;
}
template<>
bool VibConversion::decode(const Node& node, LinController& obj)
{
    obj.name = node["Name"].as<std::string>();
    optional_decode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_decode(obj.replay, node, "Replay");
    return false;
}

template<>
Node VibConversion::encode(const EthernetController& obj)
{
    Node node;
    node["Name"] = obj.name;
    node["MacAddr"] = macaddress_encode(obj.macAddress);
    if (!obj.pcapFile.empty())
    {
        node["PcapFile"] = obj.pcapFile;
    }

    if (!obj.pcapPipe.empty())
    {
        node["PcapPipe"] = obj.pcapPipe;
    }

    optional_encode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_encode(obj.replay, node, "Replay");

    return node;
}
template<>
bool VibConversion::decode(const Node& node, EthernetController& obj)
{
    obj.name = node["Name"].as<std::string>();
    obj.macAddress = macaddress_decode(node["MacAddr"]);
    optional_decode(obj.pcapFile, node, "PcapFile");
    optional_decode(obj.pcapPipe, node, "PcapPipe");
    optional_decode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_decode(obj.replay, node, "Replay");
    return true;
}

template<>
Node VibConversion::encode(const sim::fr::ClusterParameters& obj)
{
    Node node;

    node["gColdstartAttempts"] = obj.gColdstartAttempts;
    node["gCycleCountMax"] = obj.gCycleCountMax;
    node["gdActionPointOffset"] = obj.gdActionPointOffset;
    node["gdDynamicSlotIdlePhase"] = obj.gdDynamicSlotIdlePhase;
    node["gdMiniSlot"] = obj.gdMiniSlot;
    node["gdMiniSlotActionPointOffset"] = obj.gdMiniSlotActionPointOffset;
    node["gdStaticSlot"] = obj.gdStaticSlot;
    node["gdSymbolWindow"] = obj.gdSymbolWindow;
    node["gdSymbolWindowActionPointOffset"] = obj.gdSymbolWindowActionPointOffset;
    node["gdTSSTransmitter"] = obj.gdTSSTransmitter;
    node["gdWakeupTxActive"] = obj.gdWakeupTxActive;
    node["gdWakeupTxIdle"] = obj.gdWakeupTxIdle;
    node["gListenNoise"] = obj.gListenNoise;
    node["gMacroPerCycle"] = obj.gMacroPerCycle;
    node["gMaxWithoutClockCorrectionFatal"] = obj.gMaxWithoutClockCorrectionFatal;
    node["gMaxWithoutClockCorrectionPassive"] = obj.gMaxWithoutClockCorrectionPassive;
    node["gNumberOfMiniSlots"] = obj.gNumberOfMiniSlots;
    node["gNumberOfStaticSlots"] = obj.gNumberOfStaticSlots;
    node["gPayloadLengthStatic"] = obj.gPayloadLengthStatic;
    node["gSyncFrameIDCountMax"] = obj.gSyncFrameIDCountMax;
    return node;
}
template<>
bool VibConversion::decode(const Node& node, sim::fr::ClusterParameters& obj)
{
    obj.gColdstartAttempts = node["gColdstartAttempts"].as<uint8_t>();
    obj.gCycleCountMax = node["gCycleCountMax"].as<uint8_t>();
    obj.gdActionPointOffset = node["gdActionPointOffset"].as<uint16_t>();
    obj.gdDynamicSlotIdlePhase = node["gdDynamicSlotIdlePhase"].as<uint16_t>();
    obj.gdMiniSlot = node["gdMiniSlot"].as<uint16_t>();
    obj.gdMiniSlotActionPointOffset = node["gdMiniSlotActionPointOffset"].as<uint16_t>();
    obj.gdStaticSlot = node["gdStaticSlot"].as<uint16_t>();
    obj.gdSymbolWindow = node["gdSymbolWindow"].as<uint16_t>();
    obj.gdSymbolWindowActionPointOffset = node["gdSymbolWindowActionPointOffset"].as<uint16_t>();
    obj.gdTSSTransmitter = node["gdTSSTransmitter"].as<uint16_t>();
    obj.gdWakeupTxActive = node["gdWakeupTxActive"].as<uint16_t>();
    obj.gdWakeupTxIdle = node["gdWakeupTxIdle"].as<uint16_t>();
    obj.gListenNoise = node["gListenNoise"].as<uint8_t>();
    obj.gMacroPerCycle = node["gMacroPerCycle"].as<uint16_t>();
    obj.gMaxWithoutClockCorrectionFatal = node["gMaxWithoutClockCorrectionFatal"].as<uint8_t>();
    obj.gMaxWithoutClockCorrectionPassive = node["gMaxWithoutClockCorrectionPassive"].as<uint8_t>();
    obj.gNumberOfMiniSlots = node["gNumberOfMiniSlots"].as<uint16_t>();
    obj.gNumberOfStaticSlots = node["gNumberOfStaticSlots"].as<uint16_t>();
    obj.gPayloadLengthStatic = node["gPayloadLengthStatic"].as<uint16_t>();
    obj.gSyncFrameIDCountMax = node["gSyncFrameIDCountMax"].as<uint8_t>();
    return false;
}
template<>
Node VibConversion::encode(const sim::fr::NodeParameters& obj)
{
    Node node;
    node["pAllowHaltDueToClock"] = obj.pAllowHaltDueToClock;
    node["pAllowPassiveToActive"] = obj.pAllowPassiveToActive;
    node["pChannels"] = obj.pChannels;
    node["pClusterDriftDamping"] = obj.pClusterDriftDamping;
    node["pdAcceptedStartupRange"] = obj.pdAcceptedStartupRange;
    node["pdListenTimeout"] = obj.pdListenTimeout;
    node["pKeySlotId"] = obj.pKeySlotId;
    node["pKeySlotOnlyEnabled"] = obj.pKeySlotOnlyEnabled;
    node["pKeySlotUsedForStartup"] = obj.pKeySlotUsedForStartup;
    node["pKeySlotUsedForSync"] = obj.pKeySlotUsedForSync;
    node["pLatestTx"] = obj.pLatestTx;
    node["pMacroInitialOffsetA"] = obj.pMacroInitialOffsetA;
    node["pMacroInitialOffsetB"] = obj.pMacroInitialOffsetB;
    node["pMicroInitialOffsetA"] = obj.pMicroInitialOffsetA;
    node["pMicroInitialOffsetB"] = obj.pMicroInitialOffsetB;
    node["pMicroPerCycle"] = obj.pMicroPerCycle;
    node["pOffsetCorrectionOut"] = obj.pOffsetCorrectionOut;
    node["pOffsetCorrectionStart"] = obj.pOffsetCorrectionStart;
    node["pRateCorrectionOut"] = obj.pRateCorrectionOut;
    node["pWakeupChannel"] = obj.pWakeupChannel;
    node["pWakeupPattern"] = obj.pWakeupPattern;
    node["pdMicrotick"] = obj.pdMicrotick;
    node["pSamplesPerMicrotick"] = obj.pSamplesPerMicrotick;
    return node;
}
template<>
bool VibConversion::decode(const Node& node, sim::fr::NodeParameters& obj)
{
    obj.pAllowHaltDueToClock = node["pAllowHaltDueToClock"].as<uint8_t>();
    obj.pAllowPassiveToActive = node["pAllowPassiveToActive"].as<uint8_t>();
    obj.pChannels = node["pChannels"].as<sim::fr::Channel>();
    obj.pClusterDriftDamping = node["pClusterDriftDamping"].as<uint8_t>();
    obj.pdAcceptedStartupRange = node["pdAcceptedStartupRange"].as<int>();
    obj.pdListenTimeout = node["pdListenTimeout"].as<int>();
    obj.pKeySlotId = node["pKeySlotId"].as<uint16_t>();
    obj.pKeySlotOnlyEnabled = node["pKeySlotOnlyEnabled"].as<uint8_t>();
    obj.pKeySlotUsedForStartup = node["pKeySlotUsedForStartup"].as<uint8_t>();
    obj.pKeySlotUsedForSync = node["pKeySlotUsedForSync"].as<uint8_t>();
    obj.pLatestTx = node["pLatestTx"].as<uint16_t>();
    obj.pMacroInitialOffsetA = node["pMacroInitialOffsetA"].as<uint8_t>();
    obj.pMacroInitialOffsetB = node["pMacroInitialOffsetB"].as<uint8_t>();
    obj.pMicroInitialOffsetA = node["pMicroInitialOffsetA"].as<int>();
    obj.pMicroInitialOffsetB = node["pMicroInitialOffsetB"].as<int>();
    obj.pMicroPerCycle = node["pMicroPerCycle"].as<int>();
    obj.pOffsetCorrectionOut = node["pOffsetCorrectionOut"].as<int>();
    obj.pOffsetCorrectionStart = node["pOffsetCorrectionStart"].as<uint16_t>();
    obj.pRateCorrectionOut = node["pRateCorrectionOut"].as<int>();
    obj.pWakeupChannel = node["pWakeupChannel"].as<sim::fr::Channel>();
    obj.pWakeupPattern = node["pWakeupPattern"].as<uint8_t>();
    obj.pdMicrotick = node["pdMicrotick"].as<sim::fr::ClockPeriod>();
    obj.pSamplesPerMicrotick = node["pSamplesPerMicrotick"].as<uint8_t>();
    return true;
}

template<>
Node VibConversion::encode(const sim::fr::TransmissionMode& obj)
{
    Node node;

    switch (obj)
    {
    case sim::fr::TransmissionMode::Continuous:
        node = "Continuous";
        break;
    case sim::fr::TransmissionMode::SingleShot:
        node =  "SingleShot";
        break;
    default:
        throw Misconfiguration{ "Unknown TransmissionMode in VibConbersion::encode<TransmissionMode>" };
    }

    return node;
}
template<>
bool VibConversion::decode(const Node& node, sim::fr::TransmissionMode& obj)
{

    auto&& str = node.as<std::string>();
    if (str == "Continuous")
        obj = sim::fr::TransmissionMode::Continuous;
    else if (str == "SingleShot")
        obj = sim::fr::TransmissionMode::SingleShot;
    else
        return false;
    return true;

}

template<>
Node VibConversion::encode(const sim::fr::Channel& obj)
{
    Node node;

    switch (obj)
    {
    case sim::fr::Channel::A:
        node = "A";
        break;
    case sim::fr::Channel::B:
        node = "B";
        break;
    case sim::fr::Channel::AB:
        node =  "AB";
        break;
    case sim::fr::Channel::None:
        node = "None";
        break;
    }
    return node;
}
template<>
bool VibConversion::decode(const Node& node, sim::fr::Channel& obj)
{
    auto&& str = node.as<std::string>();
    if (str == "A")
        obj =  sim::fr::Channel::A;
    if (str == "B")
        obj =  sim::fr::Channel::B;
    if (str == "AB")
        obj =  sim::fr::Channel::AB;
    if (str == "None" || str == "")
        obj = sim::fr::Channel::None;
    else
        return false;
    return true;
}

template<>
Node VibConversion::encode(const sim::fr::ClockPeriod& obj)
{
    Node node;
    switch (obj)
    {
    case sim::fr::ClockPeriod::T12_5NS:
        node = "12.5ns";
        break;
    case sim::fr::ClockPeriod::T25NS:
        node =  "25ns";
        break;
    case sim::fr::ClockPeriod::T50NS:
        node = "50ns";
        break;
    default:
        throw Misconfiguration{ "Unknown ClockPeriod in VibConversion::encode<ClockPeriod>" };

    }

    return node;
}
template<>
bool VibConversion::decode(const Node& node, sim::fr::ClockPeriod& obj)
{
    auto&& str = node.as<std::string>();
    if (str == "12.5ns")
        obj = sim::fr::ClockPeriod::T12_5NS;
    else if (str == "25ns")
        obj = sim::fr::ClockPeriod::T25NS;
    else if (str == "50ns")
obj = sim::fr::ClockPeriod::T50NS;
    else
    return false;
    return true;
}

template<>
Node VibConversion::encode(const sim::fr::TxBufferConfig& obj)
{
    Node node;
    node["channels"] = obj.channels;
    node["slotId"] = obj.slotId;
    node["offset"] = obj.offset;
    node["repetition"] = obj.repetition;
    node["PPindicator"] = obj.hasPayloadPreambleIndicator;
    node["headerCrc"] = obj.headerCrc;
    node["transmissionMode"] = obj.transmissionMode;
    return node;
}
template<>
bool VibConversion::decode(const Node& node, sim::fr::TxBufferConfig& obj)
{
    obj.channels = node["channels"].as<sim::fr::Channel>();
    obj.slotId = node["slotId"].as<uint16_t>();
    obj.offset = node["offset"].as<uint8_t>();
    obj.repetition = node["repetition"].as<uint8_t>();
    obj.hasPayloadPreambleIndicator = node["PPindicator"].as<bool>();
    obj.headerCrc = node["headerCrc"].as<uint16_t>();
    obj.transmissionMode = node["transmissionMode"].as<sim::fr::TransmissionMode>();
    return true;
}

template<>
Node VibConversion::encode(const FlexrayController& obj)
{
    Node node;

    node["Name"] = obj.name;
    node["ClusterParameters"] = obj.clusterParameters;
    node["NodeParameters"] = obj.nodeParameters;
    node["TxBufferConfigs"] = obj.txBufferConfigs;
    optional_encode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_encode(obj.replay, node, "Replay");
    return node;
}
template<>
bool VibConversion::decode(const Node& node, FlexrayController& obj)
{
    obj.name = node["Name"].as<std::string>();
    optional_decode(obj.clusterParameters, node, "ClusterParameters");
    optional_decode(obj.nodeParameters, node, "NodeParameters");
    optional_decode(obj.txBufferConfigs, node, "TxBufferConfigs");
    if (node["UseTraceSinks"])
    {
        obj.useTraceSinks = node["UseTraceSinks"].as<decltype(obj.useTraceSinks)>();
    }
    optional_decode(obj.replay, node, "Replay");

    return true;
}

template<>
Node VibConversion::encode(const DigitalIoPort& obj)
{
    Node node;
    node["Name"] = obj.name;
    node["value"] = obj.initvalue;

    optional_encode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_encode(obj.replay, node, "Replay");
    return node;
}
template<>
bool VibConversion::decode(const Node& node, DigitalIoPort& obj)
{
    obj.name = node["Name"].as<std::string>();
    obj.initvalue = node["value"].as<bool>();

    optional_decode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_decode(obj.replay, node, "Replay");

    return true;
}

template<>
Node VibConversion::encode(const AnalogIoPort& obj)
{
    Node node;
    node["Name"] = obj.name;
    node["value"] = obj.initvalue;
    node["unit"] = obj.unit;

    optional_encode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_encode(obj.replay, node, "Replay");

    return node;
}
template<>
bool VibConversion::decode(const Node& node, AnalogIoPort& obj)
{
    obj.name = node["Name"].as<std::string>();
    obj.initvalue = node["value"].as<double>();
    obj.unit = node["unit"].as<std::string>();
    optional_decode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_decode(obj.replay, node, "Replay");
    return true;
}

template<>
Node VibConversion::encode(const PwmPort& obj)
{
    Node node;
    node["Name"] = obj.name;
    {
        Node freq;
        freq["value"] = obj.initvalue.frequency;
        freq["unit"] = obj.unit;
        node["freq"] = freq;
    }
    node["duty"] = obj.initvalue.dutyCycle;

    optional_encode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_encode(obj.replay, node, "Replay");
    return node;
}
template<>
bool VibConversion::decode(const Node& node, PwmPort& obj)
{
    obj.name = node["Name"].as<std::string>();
    obj.unit = node["unit"].as<std::string>();
    obj.initvalue.frequency = node["freq"]["value"].as<double>();
    obj.initvalue.dutyCycle = node["duty"].as<double>();
    optional_decode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_decode(obj.replay, node, "Replay");
    return true;
}

template<>
Node VibConversion::encode(const PatternPort& obj)
{
    Node node;
    node["Name"] = obj.name;
    node["value"] = hex_encode(obj.initvalue);
    optional_encode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_encode(obj.replay, node, "Replay");
    return node;
}
template<>
bool VibConversion::decode(const Node& node, PatternPort& obj)
{
    obj.name = node["Name"].as<std::string>();
    obj.initvalue = hex_decode(node["value"].as<std::string>());
    optional_decode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_decode(obj.replay, node, "Replay");
    return true;
}

template<>
Node VibConversion::encode(const GenericPort& obj)
{
    Node node;
    node["Name"] = obj.name;
    node["Protocol"] = obj.protocolType;
    node["DefinitionUri"] = obj.definitionUri;
    optional_encode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_encode(obj.replay, node, "Replay");
    return node;
}
template<>
bool VibConversion::decode(const Node& node, GenericPort& obj)
{
    obj.name = node["Name"].as<std::string>();
    optional_decode(obj.protocolType, node, "Protocol");
    optional_decode(obj.definitionUri, node, "DefinitionUri");
    optional_decode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_decode(obj.replay, node, "Replay");
    return true;
}

template<>
Node VibConversion::encode(const GenericPort::ProtocolType& obj)
{
    Node node;
    switch (obj)
    {
    case GenericPort::ProtocolType::ROS:
        node = "ROS";
        break;
    case GenericPort::ProtocolType::SOMEIP:
        node = "SOME/IP";
        break;
    case GenericPort::ProtocolType::Undefined:
        node = "Undefined";
        break;
    }
    return node;
}
template<>
bool VibConversion::decode(const Node& node, GenericPort::ProtocolType& obj)
{

    auto&& protocolName = node.as<std::string>();
    if (protocolName == "ROS")
        obj = GenericPort::ProtocolType::ROS;
    else if (protocolName == "SOME/IP")
        obj = GenericPort::ProtocolType::SOMEIP;
    else if (protocolName == "Undefined")
        obj = GenericPort::ProtocolType::Undefined;
    else if (protocolName == "")
        obj = GenericPort::ProtocolType::Undefined;
    else
        return false;
    return true;
}

template<>
Node VibConversion::encode(const SyncType& obj)
{
    Node node;
    node = to_string(obj);
    return node;
}
template<>
bool VibConversion::decode(const Node& node, SyncType& obj)
{
    auto&& syncType = node.as<std::string>();

    if (syncType == "DistributedTimeQuantum")
        obj = SyncType::DistributedTimeQuantum;
    else if (syncType == "DiscreteEvent")
        obj = SyncType::DiscreteEvent;
    else if (syncType == "TimeQuantum")
        obj = SyncType::TimeQuantum;
    else if (syncType == "DiscreteTime")
        obj = SyncType::DiscreteTime;
    else if (syncType == "DiscreteTimePassive")
        obj = SyncType::DiscreteTimePassive;
    else if (syncType == "Unsynchronized")
        obj = SyncType::Unsynchronized;
    else 
        return false;
    return true;
}

template<>
Node VibConversion::encode(const ParticipantController& obj)
{

    Node node;
    node["SyncType"] = obj.syncType;
    auto optional_duration = [&node](auto name, auto x) {
        if (x != decltype(x)::max())
            node[name] = std::chrono::duration_cast<std::chrono::milliseconds>(x).count();
    };
    optional_duration("ExecTimeLimitSoftMs", obj.execTimeLimitSoft);
    optional_duration("ExecTimeLimitHardMs", obj.execTimeLimitHard);
    return node;
}
template<>
bool VibConversion::decode(const Node& node, ParticipantController& obj)
{
    obj.syncType = node["SyncType"].as<SyncType>();
    optional_decode(obj.execTimeLimitHard, node, "ExecTimeLimitHardMs");
    optional_decode(obj.execTimeLimitSoft, node, "ExecTimeLimitSoftMs");
    return true;
}

template<>
Node VibConversion::encode(const std::chrono::milliseconds& obj)
{
    Node node;
    node = obj.count();
    return node;
}
template<>
bool VibConversion::decode(const Node& node, std::chrono::milliseconds& obj)
{
    obj = std::chrono::milliseconds{node.as<uint64_t>()};
    return true;
}

template<>
Node VibConversion::encode(const std::chrono::nanoseconds& obj)
{
    Node node;
    node = obj.count();
    return node;
}
template<>
bool VibConversion::decode(const Node& node, std::chrono::nanoseconds& obj)
{
    obj = std::chrono::nanoseconds{node.as<uint64_t>()};
    return true;
}

template<>
Node VibConversion::encode(const Participant& obj)
{
    auto makePortList = [](auto&& portVector, PortDirection direction)
    {
        YAML::Node sequence;
        for (auto&& port : portVector)
        {
            if (port.direction == direction)
            {
                sequence.push_back(port);
            }
        }
        return sequence;
    };

    Node node;
    node["Name"] = obj.name;
    node["Logger"] = obj.logger;

    node["CanControllers"] = obj.canControllers;
    node["LinControllers"] = obj.linControllers;
    node["EthernetControllers"] = obj.ethernetControllers;
    node["FlexRayControllers"] = obj.flexrayControllers;
    node["NetworkSimulators"] = obj.networkSimulators;

    node["Analog-In"] = makePortList(obj.analogIoPorts, PortDirection::In);
    node["Digital-In"] = makePortList(obj.digitalIoPorts, PortDirection::In);
    node["Pwm-In"] = makePortList(obj.pwmPorts, PortDirection::In);
    node["Pattern-In"] = makePortList(obj.patternPorts, PortDirection::In);
    node["Analog-Out"] = makePortList(obj.analogIoPorts, PortDirection::Out);
    node["Digital-Out"] = makePortList(obj.digitalIoPorts, PortDirection::Out);
    node["Pwm-Out"] = makePortList(obj.pwmPorts, PortDirection::Out);
    node["Pattern-Out"] = makePortList(obj.patternPorts, PortDirection::Out);

    node["GenericPublishers"] = obj.genericPublishers;
    node["GenericSubscribers"] = obj.genericSubscribers;

    node["TraceSinks"] = obj.traceSinks;
    node["TraceSources"] = obj.traceSources;
    node["IsSyncMaster"] = obj.isSyncMaster;
    optional_encode(obj.participantController, node, "ParticipantController");

    return node;
}
template<>
bool VibConversion::decode(const Node& node, Participant& obj)
{
    // we need to explicitly set the direction for the parsed port
    auto makePorts = [&node](auto&& portList, auto&& propertyName, auto&& direction)
    {
        using PortList = typename std::decay_t<decltype(portList)>;
        using PortType = typename std::decay_t<decltype(portList)>::value_type;

        PortList ports;
        optional_decode(ports, node, propertyName);
        for (auto& port : ports) {
            port.direction = direction;
            portList.push_back(port);
        }
    };

    obj.name = node["Name"].as<std::string>();

    optional_decode(obj.logger, node, "Logger");
    optional_decode(obj.isSyncMaster, node, "IsSyncMaster");
    optional_decode(obj.participantController, node, "ParticipantController");
    optional_decode(obj.canControllers, node, "CanControllers");
    optional_decode(obj.linControllers, node, "LinControllers");
    optional_decode(obj.ethernetControllers, node, "EthernetControllers");
    optional_decode(obj.flexrayControllers, node, "FlexRayControllers");
    optional_decode(obj.networkSimulators, node, "NetworkSimulators");

    optional_decode(obj.genericPublishers, node, "GenericPublishers");
    optional_decode(obj.genericSubscribers, node, "GenericSubscribers");

    optional_decode(obj.traceSinks, node, "TraceSinks");
    optional_decode(obj.traceSources, node, "TraceSources");

    makePorts(obj.digitalIoPorts, "Digital-Out", PortDirection::Out);
    makePorts(obj.digitalIoPorts, "Digital-In", PortDirection::In);
    makePorts(obj.analogIoPorts, "Analog-Out", PortDirection::Out);
    makePorts(obj.analogIoPorts, "Analog-In", PortDirection::In);
    makePorts(obj.pwmPorts, "Pwm-Out", PortDirection::Out);
    makePorts(obj.pwmPorts, "Pwm-In", PortDirection::In);
    makePorts(obj.patternPorts, "Pattern-Out", PortDirection::Out);
    makePorts(obj.patternPorts, "Pattern-In", PortDirection::In);


    return true;
}

template<>
Node VibConversion::encode(const Switch::Port& obj)
{
    Node node;
    node["Name"] = obj.name;
    node["VlanIds"] = obj.vlanIds;
    return node;
}
template<>
bool VibConversion::decode(const Node& node, Switch::Port& obj)
{
    obj.name = node["Name"].as<std::string>();
    optional_decode(obj.vlanIds, node, "VlanIds");
    return true;
}

template<>
Node VibConversion::encode(const Switch& obj)
{
    Node node;
    node["Name"] = obj.name;
    node["Description"] = obj.description;
    node["Ports"] = obj.ports;
    return node;
}
template<>
bool VibConversion::decode(const Node& node, Switch& obj)
{
    obj.name = node["Name"].as<std::string>();
    obj.description = node["Description"].as<std::string>();
    optional_decode(obj.ports, node, "Ports");
    return true;
}

template<>
Node VibConversion::encode(const Link& obj)
{
    Node node;
    node["Name"] = obj.name;
    node["Endpoints"] = obj.endpoints;
    return node;
}
template<>
bool VibConversion::decode(const Node& node, Link& obj)
{
    obj.name = node["Name"].as<std::string>();
    obj.name = node["Endpoints"].as<std::string>();
    return true;
}

template<>
Node VibConversion::encode(const NetworkSimulator& obj)
{
    Node node;
    node["Name"] = obj.name;
    node["SimulatedLinks"] = obj.simulatedLinks;
    node["SimulatedSwitches"] = obj.simulatedSwitches;
    optional_encode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_encode(obj.replay, node, "Replay");
    return node;
}
template<>
bool VibConversion::decode(const Node& node, NetworkSimulator& obj)
{
    obj.name = node["Name"].as<std::string>();
    optional_decode(obj.simulatedSwitches, node, "SimulatedSwitches");
    optional_decode(obj.simulatedLinks, node, "SimulatedLinks");
    optional_decode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_decode(obj.replay, node, "Replay");
    return true;
}

template<>
Node VibConversion::encode(const TimeSync::SyncPolicy& obj)
{
    Node node;
    try
    {
        node = to_string(obj);
    }
    catch (const ib::type_conversion_error&)
    {
        node = "UNKNOWN_SYNC_POLICY";
    }
    return node;
}
template<>
bool VibConversion::decode(const Node& node, TimeSync::SyncPolicy& obj)
{
    auto&& syncType = node.as<std::string>();
    if (syncType == "Loose")
        obj = TimeSync::SyncPolicy::Loose;
    else if (syncType == "Strict")
        obj = TimeSync::SyncPolicy::Strict;
    else
        return false;
    return true;
}

template<>
Node VibConversion::encode(const TimeSync& obj)
{
    Node node;
    node["SyncPolicy"] = obj.syncPolicy;
    node["TickPeriodNs"] = obj.tickPeriod;
    return node;
}
template<>
bool VibConversion::decode(const Node& node, TimeSync& obj)
{
    optional_decode(obj.syncPolicy, node, "SyncPolicy");
    obj.tickPeriod = node["TickPeriodNs"].as<std::chrono::nanoseconds>();
    return true;
}

template<>
Node VibConversion::encode(const SimulationSetup& obj)
{
    Node node;
    node["Participants"] = obj.participants;
    node["Switches"] = obj.switches;
    node["Links"] = obj.links;
    node["TimeSync"] = obj.timeSync;
    return node;
}
template<>
bool VibConversion::decode(const Node& node, SimulationSetup& obj)
{
    obj.participants = node["Participants"].as<decltype(obj.participants)>();
    obj.switches = node["Switches"].as<decltype(obj.switches)>();
    obj.links = node["Links"].as<decltype(obj.links)>();
    obj.timeSync = node["TimeSync"].as<decltype(obj.timeSync)>();
    return true;
}

template<>
Node VibConversion::encode(const FastRtps::DiscoveryType& obj)
{
    Node node;
    try
    {
        node = to_string(obj);
    }
    catch (const ib::type_conversion_error&)
    {
        node = "UNKNOWN_DISCOVERY_TYPE";
    }
    return node;
}
template<>
bool VibConversion::decode(const Node& node, FastRtps::DiscoveryType& obj)
{
    auto&& discoveryType = node.as<std::string>();

    if (discoveryType == "Local")
        obj = FastRtps::DiscoveryType::Local;
    else if (discoveryType == "Multicast")
        obj = FastRtps::DiscoveryType::Multicast;
    else if (discoveryType == "Unicast")
        obj = FastRtps::DiscoveryType::Unicast;
    else if (discoveryType == "ConfigFile")
        obj = FastRtps::DiscoveryType::ConfigFile;
    else
        return false;
    return true;
}

template<>
Node VibConversion::encode(const FastRtps::Config& obj)
{
    Node node;
    node["DiscoveryType"] = obj.discoveryType;
    node["UnicastLocators"] = obj.unicastLocators;
    node["ConfigFileName"] = obj.configFileName;
    auto optional_field = [&node](auto value, auto name) {
        if (value != -1)
            node[name] = value;
    };
    optional_field(obj.sendSocketBufferSize, "SendSocketBufferSize");
    optional_field(obj.listenSocketBufferSize, "ListenSocketBufferSize");
    optional_field(obj.historyDepth, "HistoryDepth");
    return node;
}
template<>
bool VibConversion::decode(const Node& node, FastRtps::Config& obj)
{

    optional_decode(obj.discoveryType, node, "DiscoveryType");
    obj.configFileName = node["ConfigFileName"].as<decltype(obj.configFileName)>();
    obj.unicastLocators = node["UnicastLocators"].as<decltype(obj.unicastLocators)>();

    switch (obj.discoveryType)
    {
    case FastRtps::DiscoveryType::Local:
        if (!obj.unicastLocators.empty())
            return false; // Misconfiguration{ "UnicastLocators must not be specified when using DiscoveryType Local" };

        if (!obj.configFileName.empty())
            return false; // Misconfiguration{ "Using a FastRTPS configuration file requires DiscoverType ConfigFile" };

        break;

    case FastRtps::DiscoveryType::Multicast:
        if (!obj.unicastLocators.empty())
            return false; // throw Misconfiguration{ "UnicastLocators must not be specified when using DiscoveryType Multicast" };

        if (!obj.configFileName.empty())
            return false; // throw Misconfiguration{ "Using a FastRTPS configuration file requires DiscoverType ConfigFile" };

        break;

    case FastRtps::DiscoveryType::Unicast:
        if (obj.unicastLocators.empty())
            return false; // throw Misconfiguration{ "DiscoveryType Unicast requires UnicastLocators being specified" };

        if (!obj.configFileName.empty())
            return false; // throw Misconfiguration{ "Using a FastRTPS configuration file requires DiscoverType ConfigFile" };

        break;

    case FastRtps::DiscoveryType::ConfigFile:
        if (!obj.unicastLocators.empty())
            return false; // throw Misconfiguration{ "UnicastLocators must not be specified when using DiscoveryType Multicast" };

        if (obj.configFileName.empty())
            return false; // Misconfiguration{ "DiscoveryType ConfigFile requires ConfigFileName being specified" };

        break;

    default:
        return false; // Misconfiguration{ "Invalid FastRTPS discovery type: " + to_string(fastRtps.discoveryType) };
    }

    optional_decode(obj.sendSocketBufferSize, node, "SendSocketBufferSize");
    optional_decode(obj.listenSocketBufferSize, node, "ListenSocketBufferSize");
    optional_decode(obj.historyDepth, node, "HistoryDepth");
    if (node["HistoryDepth"])
    {
        if (obj.historyDepth <= 0)
            return false; // throw Misconfiguration{ "FastRTPS HistoryDepth must be above 0" };
    }
    return true;
}

template<>
Node VibConversion::encode(const VAsio::RegistryConfig& obj)
{
    static const VAsio::RegistryConfig defaultObj;
    Node node;
    non_default_encode(obj.hostname, node, "Hostname", defaultObj.hostname);
    non_default_encode(obj.port, node, "Port", defaultObj.port);
    non_default_encode(obj.logger, node, "Logger", defaultObj.logger);
    non_default_encode(obj.connectAttempts, node, "ConnectAttempts", defaultObj.connectAttempts);
    return node;
}
template<>
bool VibConversion::decode(const Node& node, VAsio::RegistryConfig& obj)
{
    optional_decode(obj.hostname, node, "Hostname");
    optional_decode(obj.port, node, "Port");
    optional_decode(obj.logger, node, "Logger");
    optional_decode(obj.connectAttempts, node, "ConnectAttempts");
    return false;
}

template<>
Node VibConversion::encode(const VAsio::Config& obj)
{
    Node node;
    static const VAsio::Config defaultObj;

    non_default_encode(obj.registry, node, "Registry", defaultObj.registry);
    non_default_encode(obj.tcpNoDelay, node, "TcpNoDelay", defaultObj.tcpNoDelay);
    non_default_encode(obj.tcpQuickAck, node, "TcpQuickAck", defaultObj.tcpQuickAck);
    non_default_encode(obj.tcpReceiveBufferSize, node, "TcpReceiveBufferSize", defaultObj.tcpReceiveBufferSize);
    non_default_encode(obj.tcpSendBufferSize, node, "TcpSendBufferSize", defaultObj.tcpSendBufferSize);
    return node;
}
template<>
bool VibConversion::decode(const Node& node, VAsio::Config& obj)
{
    optional_decode(obj.registry, node, "Registry");
    optional_decode(obj.tcpNoDelay, node, "TcpNoDelay");
    optional_decode(obj.tcpQuickAck, node, "TcpQuickAck");
    optional_decode(obj.tcpReceiveBufferSize, node, "TcpReceiveBufferSize");
    optional_decode(obj.tcpSendBufferSize, node, "TcpSendBufferSize");
    return true;
}

template<>
Node VibConversion::encode(const Middleware& obj)
{
    Node node;
    try
    {
        node = to_string(obj);
    }
    catch (const ib::type_conversion_error&)
    {
        node = "UNKNOWN_MIDDLEWARE";
    }
    return node;
}
template<>
bool VibConversion::decode(const Node& node, Middleware& obj)
{
    try
    {
        obj = from_string<Middleware>(node.as<std::string>());
    }
    catch (const type_conversion_error&)
    {
        return false; // throw Misconfiguration{ "Unknown active middleware in from_json" };
    }
}

template<>
Node VibConversion::encode(const MiddlewareConfig& obj)
{
    const static MiddlewareConfig defaultObj{};
    Node node;
    non_default_encode(obj.activeMiddleware, node, "ActiveMiddleware", defaultObj.activeMiddleware);
    non_default_encode(obj.fastRtps, node, "FastRtps", defaultObj.fastRtps);
    non_default_encode(obj.vasio, node, "VAsio", defaultObj.vasio);
    return node;
}

template<>
bool VibConversion::decode(const Node& node, MiddlewareConfig& obj)
{
    optional_decode(obj.activeMiddleware, node, "ActiveMiddleware");
    optional_decode(obj.fastRtps, node, "FastRtps");
    optional_decode(obj.vasio, node, "VAsio");
    return true;
}

template<>
Node VibConversion::encode(const ExtensionConfig& obj)
{
    static const ExtensionConfig defaultObj;
    Node node;
    non_default_encode(obj.searchPathHints, node, "SearchPathHints", defaultObj.searchPathHints);
    return node;
}
template<>
bool VibConversion::decode(const Node& node, ExtensionConfig& obj)
{
    optional_decode(obj.searchPathHints, node, "SearchPathHints");
    return true;
}

template<>
Node VibConversion::encode(const Config& obj)
{
    static const Config defaultObj;
    Node node;
    node["ConfigVersion"] = obj.version;
    node["ConfigName"] = obj.name;
    node["Description"] = obj.description;
    node["SimulationSetup"] = obj.simulationSetup;
    node["MiddlewareConfig"] = obj.middlewareConfig;
    non_default_encode(obj.extensionConfig, node, "ExtensionConfig", defaultObj.extensionConfig);
    return node;
}
template<>
bool VibConversion::decode(const Node& node, Config& obj)
{
    obj.version = node["ConfigVersion"].as<decltype(obj.version)>();
    obj.name = node["ConfigName"].as<decltype(obj.name)>();
    obj.description = node["Description"].as<decltype(obj.description)>();
    obj.simulationSetup = node["SimulationSetup"].as<decltype(obj.simulationSetup)>();
    obj.middlewareConfig = node["MiddlewareConfig"].as<decltype(obj.middlewareConfig)>();
    optional_decode(obj.extensionConfig, node, "ExtensionConfig");
    return true;
}

template<>
Node VibConversion::encode(const TraceSink& obj)
{
    Node node;
    node["Name"] = obj.name;
    node["Type"] = obj.type;
    node["OutputPath"] = obj.outputPath;
    //only serialize if disabled
    if (!obj.enabled)
    {
        node["Enabled"] = obj.enabled;
    }

    return node;
}
template<>
bool VibConversion::decode(const Node& node, TraceSink& obj)
{
    obj.name = node["Name"].as<std::string>();
    obj.type = node["Type"].as<decltype(obj.type)>();
    obj.outputPath = node["OutputPath"].as<decltype(obj.outputPath)>();
    if (node["Enabled"])
    {
        obj.enabled = node["Enabled"].as<decltype(obj.enabled)>();
    }
    return true;
}

template<>
Node VibConversion::encode(const TraceSink::Type& obj)
{
    Node node;
    switch (obj)
    {
    case TraceSink::Type::Undefined:
        node =  "Undefined";
        break;
    case TraceSink::Type::Mdf4File:
        node = "Mdf4File";
        break;
    case TraceSink::Type::PcapFile:
        node = "PcapFile";
        break;
    case TraceSink::Type::PcapPipe:
        node = "PcapPipe";
        break;
    default:
        throw Misconfiguration{"Unknown TraceSink Type"};
    }
    return node;
}
template<>
bool VibConversion::decode(const Node& node, TraceSink::Type& obj)
{
    auto&& str = node.as<std::string>();
    if (str == "Undefined" || str == "")
        obj = TraceSink::Type::Undefined;
    else if (str == "Mdf4File")
        obj = TraceSink::Type::Mdf4File;
    else if (str == "PcapFile")
        obj = TraceSink::Type::PcapFile;
    else if (str == "PcapPipe")
        obj = TraceSink::Type::PcapPipe;
    else
        return false;
    return true;
}

template<>
Node VibConversion::encode(const TraceSource& obj)
{
    Node node;
    return node;
}
template<>
bool VibConversion::decode(const Node& node, TraceSource& obj)
{
    return false;
}

template<>
Node VibConversion::encode(const TraceSource::Type& obj)
{
    Node node;
    switch (obj)
    {
    case TraceSource::Type::Undefined:
        node = "Undefined";
        break;
    case TraceSource::Type::Mdf4File:
        node = "Mdf4File";
        break;
    case TraceSource::Type::PcapFile:
        node = "PcapFile";
        break;
    default:
        throw Misconfiguration{"Unknown TraceSource Type"};
    }
    return node;
}
template<>
bool VibConversion::decode(const Node& node, TraceSource::Type& obj)
{
    auto&& str = node.as<std::string>();
    if (str == "Undefined" || str == "")
        obj = TraceSource::Type::Undefined;
    else if (str == "Mdf4File")
        obj = TraceSource::Type::Mdf4File;
    else if (str == "PcapFile")
        obj = TraceSource::Type::PcapFile;
    else 
        return false;
    return true;
}

template<>
Node VibConversion::encode(const Replay& obj)
{
    static const Replay defaultObj;
    Node node;
    node["UseTraceSource"] = obj.useTraceSource;
    non_default_encode(obj.direction, node, "Direction", defaultObj.direction);
    non_default_encode(obj.mdfChannel, node, "MdfChannel", defaultObj.mdfChannel);

    return node;
}
template<>
bool VibConversion::decode(const Node& node, Replay& obj)
{
    obj.useTraceSource = node["UseTraceSource"].as<decltype(obj.useTraceSource)>();
    optional_decode(obj.direction, node, "Direction");
    optional_decode(obj.mdfChannel, node, "MdfChannel");
    return true;
}

template<>
Node VibConversion::encode(const Replay::Direction& obj)
{
    Node node;
    return node;
}
template<>
bool VibConversion::decode(const Node& node, Replay::Direction& obj)
{
    return false;
}

} // namespace YAML
