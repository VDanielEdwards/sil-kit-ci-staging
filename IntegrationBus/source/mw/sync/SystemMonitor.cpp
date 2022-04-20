// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include "SystemMonitor.hpp"
#include "IServiceDiscovery.hpp"
#include "ParticipantController.hpp"

#include <algorithm>
#include <ctime>
#include <iomanip> //std:put_time

#include "ib/mw/logging/ILogger.hpp"
#include "ib/mw/sync/string_utils.hpp"

namespace ib {
namespace mw {
namespace sync {

SystemMonitor::SystemMonitor(IParticipantInternal* participant)
    : _participant{participant}
    , _logger{participant->GetLogger()}
{
}

const ib::mw::sync::ExpectedParticipants& SystemMonitor::GetExpectedParticipants() const
{
    return _expectedParticipants;
}

void SystemMonitor::ReceiveIbMessage(const IIbServiceEndpoint* /*from*/,
                                     const sync::ExpectedParticipants& expectedParticipants)
{
    UpdateExpectedParticipantNames(expectedParticipants);
}

void SystemMonitor::UpdateExpectedParticipantNames(const ExpectedParticipants& expectedParticipants)
{
    // Prevent calling this method more than once
    if (!_expectedParticipants.names.empty())
    {
        throw std::runtime_error{"Expected participant names are already set."};
    }

    _expectedParticipants.names = expectedParticipants.names;

    bool anyParticipantStateReceived = false;
    // Init new participants in status map
    for (auto&& name : _expectedParticipants.names)
    {
        auto&& statusIter = _participantStatus.find(name);
        if (statusIter == _participantStatus.end())
        {
            _participantStatus[name] = sync::ParticipantStatus{};
            _participantStatus[name].state = sync::ParticipantState::Invalid;
        }
        else
        {
            anyParticipantStateReceived = true;
        }
    }

    // Update / propagate the system state in case ParticipantStatus have been received already
    if (anyParticipantStateReceived)
    {
        auto oldSystemState = _systemState;
        for (auto&& name : _expectedParticipants.names)
        {
            UpdateSystemState(_participantStatus[name]);
        }
        if (oldSystemState != _systemState)
        {
            for (auto&& handler : _systemStateHandlers)
                handler(_systemState);
        }
    }

    // Add expected participants in sync policy of participantController
    // if this participant is synchronized and found in expectedNames
    if (_participant->IsSynchronized())
    {
        auto&& nameIter =
            std::find(_expectedParticipants.names.begin(), _expectedParticipants.names.end(), _participant->GetParticipantName());
        if (nameIter != _expectedParticipants.names.end())
        {
            auto* participantController = dynamic_cast<ib::mw::sync::ParticipantController*>(_participant->GetParticipantController());
            if (participantController)
            {
                participantController->AddSynchronizedParticipants(expectedParticipants);
            }
        }
    }
}



void SystemMonitor::RegisterSystemStateHandler(SystemStateHandlerT handler)
{
    _systemStateHandlers.emplace_back(std::move(handler));

    if (_systemState != sync::SystemState::Invalid)
    {
        auto&& newHandler = _systemStateHandlers[_systemStateHandlers.size() - 1];
        newHandler(_systemState);
    }
}

void SystemMonitor::RegisterParticipantStatusHandler(ParticipantStatusHandlerT handler)
{
    _participantStatusHandlers.emplace_back(std::move(handler));

    auto&& newHandler = _participantStatusHandlers[_participantStatusHandlers.size() - 1];
    for (auto&& kv : _participantStatus)
    {
        auto&& participantStatus = kv.second;
        if (participantStatus.state == sync::ParticipantState::Invalid)
            continue;

        newHandler(participantStatus);
    }
}

auto SystemMonitor::SystemState() const -> sync::SystemState
{
    return _systemState;
}

auto SystemMonitor::ParticipantStatus(const std::string& participantName) const -> const sync::ParticipantStatus&
{
    auto&& statusIter = _participantStatus.find(participantName);
    if (statusIter == _participantStatus.end())
    {
        throw std::runtime_error{"Unknown participantName"};
    }

    return statusIter->second;
}

void SystemMonitor::ReceiveIbMessage(const IIbServiceEndpoint* /*from*/, const sync::ParticipantStatus& newParticipantStatus)
{
    auto participantName = newParticipantStatus.participantName;

    // Validation
    auto oldParticipantState = _participantStatus[participantName].state;
    ValidateParticipantStatusUpdate(newParticipantStatus, oldParticipantState);
    if (oldParticipantState == sync::ParticipantState::Shutdown)
    {
        _logger->Debug("Ignoring ParticipantState update from participant {} to ParticipantState::{} because "
                       "participant is already in terminal state ParticipantState::Shutdown.",
                       newParticipantStatus.participantName, newParticipantStatus.state);
        return;
    }

    // Update status map
    _participantStatus[participantName] = newParticipantStatus;

    // On new participant state
    if (oldParticipantState != newParticipantStatus.state)
    {
        // Fire status / state handler
        for (auto&& handler : _participantStatusHandlers)
            handler(newParticipantStatus);

        // Propagate the system state for known participants, ignore otherwise
        auto&& nameIter = std::find(_expectedParticipants.names.begin(), _expectedParticipants.names.end(), participantName);
        if (nameIter != _expectedParticipants.names.end())
        {
            auto oldSystemState = _systemState;
            UpdateSystemState(newParticipantStatus);

            if (oldSystemState != _systemState)
            {
                for (auto&& handler : _systemStateHandlers)
                    handler(_systemState);
            }
        }
    }
}

bool SystemMonitor::AllParticipantsInState(sync::ParticipantState state) const
{
    return std::all_of(begin(_participantStatus), end(_participantStatus), [state](auto&& kv) {
        return kv.second.state == state;
    });
}

bool SystemMonitor::AllParticipantsInState(std::initializer_list<sync::ParticipantState> acceptedStates) const
{
    for (auto&& participantStatus : _participantStatus)
    {
        bool isAcceptedState = std::any_of(begin(acceptedStates), end(acceptedStates), [participantState = participantStatus.second.state](auto acceptedState) {
            return participantState == acceptedState;
        });
        if (!isAcceptedState)
            return false;
    }
    return true;
}

void SystemMonitor::ValidateParticipantStatusUpdate(const sync::ParticipantStatus& newStatus, sync::ParticipantState oldState)
{
    auto is_any_of = [](sync::ParticipantState state, std::initializer_list<sync::ParticipantState> stateList)
    {
        return std::any_of(begin(stateList), end(stateList), [=](auto candidate) { return candidate == state; });
    };

    switch (newStatus.state)
    {
    case sync::ParticipantState::Idle:
        if (is_any_of(oldState, {sync::ParticipantState::Invalid, sync::ParticipantState::ColdswapShutdown}))
            return;

    case sync::ParticipantState::Initializing:
        if (is_any_of(oldState, {sync::ParticipantState::Idle, sync::ParticipantState::Error, sync::ParticipantState::Stopped, sync::ParticipantState::ColdswapIgnored}))
            return;

    case sync::ParticipantState::Initialized:
        if (oldState == sync::ParticipantState::Initializing)
            return;

    case sync::ParticipantState::Running:
        if (is_any_of(oldState, {sync::ParticipantState::Initialized, sync::ParticipantState::Paused}))
            return;

    case sync::ParticipantState::Paused:
        if (oldState == sync::ParticipantState::Running)
            return;

    case sync::ParticipantState::Stopping:
        if (is_any_of(oldState, {sync::ParticipantState::Running, sync::ParticipantState::Paused}))
            return;

    case sync::ParticipantState::Stopped:
        if (oldState == sync::ParticipantState::Stopping)
            return;

    case sync::ParticipantState::ColdswapPrepare:
        if (is_any_of(oldState, {sync::ParticipantState::Stopped, sync::ParticipantState::Error}))
            return;

    case sync::ParticipantState::ColdswapReady:
        if (oldState == sync::ParticipantState::ColdswapPrepare)
            return;

    case sync::ParticipantState::ColdswapIgnored:
        if (oldState == sync::ParticipantState::ColdswapReady)
            return;

    case sync::ParticipantState::ColdswapShutdown:
        if (oldState == sync::ParticipantState::ColdswapReady)
            return;

    case sync::ParticipantState::ShuttingDown:
        if (is_any_of(oldState, {sync::ParticipantState::Error, sync::ParticipantState::Stopped}))
            return;

    case sync::ParticipantState::Shutdown:
        if (oldState == sync::ParticipantState::ShuttingDown)
            return;

    case sync::ParticipantState::Error:
        return;

    default:
        _logger->Error("SystemMonitor::ValidateParticipantStatusUpdate() Unhandled ParticipantState::{}", newStatus.state);
    }

    std::time_t enterTime = std::chrono::system_clock::to_time_t(newStatus.enterTime);
    std::tm tmBuffer;
#if defined(_WIN32)
    localtime_s(&tmBuffer, &enterTime);
#else
    localtime_r(&enterTime, &tmBuffer);
#endif
    std::stringstream timeBuf;
    timeBuf <<  std::put_time(&tmBuffer, "%FT%T");

    _logger->Error(
        "SystemMonitor detected invalid ParticipantState transition from {} to {} EnterTime={}, EnterReason=\"{}\"",
        oldState,
        newStatus.state,
        timeBuf.str(),
        newStatus.enterReason);

    _invalidTransitionCount++;
}

void SystemMonitor::UpdateSystemState(const sync::ParticipantStatus& newStatus)
{
    switch (newStatus.state)
    {
    case sync::ParticipantState::Idle:
        if (SystemState() == sync::SystemState::ColdswapPending)
        {
            if (AllParticipantsInState({sync::ParticipantState::Idle, sync::ParticipantState::ColdswapIgnored}))
            {
                SetSystemState(sync::SystemState::ColdswapDone);
            }

        }
        else
        {
            if (AllParticipantsInState(sync::ParticipantState::Idle))
                SetSystemState(sync::SystemState::Idle);
            else if (AllParticipantsInState({sync::ParticipantState::Idle, sync::ParticipantState::Initializing, sync::ParticipantState::Initialized}))
                SetSystemState(sync::SystemState::Initializing);
        }
        return;

    case sync::ParticipantState::Initializing:
        if (AllParticipantsInState({sync::ParticipantState::Initializing, sync::ParticipantState::Idle}) // regular start
            || AllParticipantsInState({sync::ParticipantState::Initializing, sync::ParticipantState::Stopped, sync::ParticipantState::Error}) // re-initialization after one run
            || AllParticipantsInState({sync::ParticipantState::Initializing, sync::ParticipantState::Idle, sync::ParticipantState::ColdswapIgnored}) // after coldswap
            )
        {
            SetSystemState(sync::SystemState::Initializing);
        }
        return;

    case sync::ParticipantState::Initialized:
        if (AllParticipantsInState(sync::ParticipantState::Initialized))
            SetSystemState(sync::SystemState::Initialized);

        return;

    case sync::ParticipantState::Running:
        if (AllParticipantsInState(sync::ParticipantState::Running))
            SetSystemState(sync::SystemState::Running);
        return;

    case sync::ParticipantState::Paused:
        if (AllParticipantsInState({sync::ParticipantState::Paused, sync::ParticipantState::Running}))
            SetSystemState(sync::SystemState::Paused);
        return;

    case sync::ParticipantState::Stopping:
        if (AllParticipantsInState({sync::ParticipantState::Stopping, sync::ParticipantState::Stopped, sync::ParticipantState::Paused, sync::ParticipantState::Running}))
            SetSystemState(sync::SystemState::Stopping);

    case sync::ParticipantState::Stopped:
        if (AllParticipantsInState(sync::ParticipantState::Stopped))
            SetSystemState(sync::SystemState::Stopped);
        return;

    case sync::ParticipantState::ColdswapPrepare:
        if (AllParticipantsInState({sync::ParticipantState::Stopped, sync::ParticipantState::ColdswapPrepare, sync::ParticipantState::ColdswapReady, sync::ParticipantState::Error}))
            SetSystemState(sync::SystemState::ColdswapPrepare);
        return;

    case sync::ParticipantState::ColdswapReady:
        if (AllParticipantsInState(sync::ParticipantState::ColdswapReady))
            SetSystemState(sync::SystemState::ColdswapReady);
        return;

    case sync::ParticipantState::ColdswapShutdown:
        if (AllParticipantsInState({sync::ParticipantState::ColdswapReady, sync::ParticipantState::ColdswapShutdown, sync::ParticipantState::ColdswapIgnored}))
            SetSystemState(sync::SystemState::ColdswapPending);
        return;

    case sync::ParticipantState::ColdswapIgnored:
        if (AllParticipantsInState(sync::ParticipantState::ColdswapIgnored))
            SetSystemState(sync::SystemState::ColdswapDone);
        else if (AllParticipantsInState({sync::ParticipantState::ColdswapReady, sync::ParticipantState::ColdswapShutdown, sync::ParticipantState::ColdswapIgnored}))
            SetSystemState(sync::SystemState::ColdswapPending);
        return;

    case sync::ParticipantState::ShuttingDown:
        if (AllParticipantsInState({sync::ParticipantState::ShuttingDown, sync::ParticipantState::Shutdown, sync::ParticipantState::Stopped, sync::ParticipantState::Error, sync::ParticipantState::Idle, sync::ParticipantState::Initialized}))
            SetSystemState(sync::SystemState::ShuttingDown);

    case sync::ParticipantState::Shutdown:
        if (AllParticipantsInState(sync::ParticipantState::Shutdown))
            SetSystemState(sync::SystemState::Shutdown);
        return;

    case sync::ParticipantState::Error:
        SetSystemState(sync::SystemState::Error);
        return;

    default:
        return;
    }
}

void SystemMonitor::SetSystemState(sync::SystemState newState)
{
    _systemState = newState;
}


} // namespace sync
} // namespace mw
} // namespace ib
