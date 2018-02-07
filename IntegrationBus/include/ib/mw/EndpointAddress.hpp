// Copyright (c)  Vector Informatik GmbH. All rights reserved.

#pragma once

#include <cstdint>

namespace ib {
namespace mw {

/*! \brief Global identifier for Integration Bus participants
 *
 *  The execution of IB participants is governed by the central
 *  /ExecutionController/. Particpants must interact with the controller
 *  via ISyncAdapter::SynchronizeDT or ISyncAdapter::SynchronizeET but
 *  not both.
*/
using ParticipantId = uint16_t;

/*! \brief Participant specific identifier for its communication endpoints.
 *
 *   One Integration Bus participant can have multiple communication objects,
 *   e.g., multiple CAN controllers attached to differen buses. A EndpointId
 *   is only valid in the scope of a specific Instruction Bus participant.
 */
using EndpointId = uint16_t;

/*! \brief Global address of a ComEndpoint
 *
 *  Pair of ParticipantId and EndpointId.
 */
struct EndpointAddress
{
    ParticipantId participant;
    EndpointId endpoint;
};

inline bool operator==(ib::mw::EndpointAddress lhs, ib::mw::EndpointAddress rhs);
inline bool operator!=(ib::mw::EndpointAddress lhs, ib::mw::EndpointAddress rhs);


// ================================================================================
//  Inline Implementations
// ================================================================================
bool operator==(ib::mw::EndpointAddress lhs, ib::mw::EndpointAddress rhs)
{
    return lhs.participant == rhs.participant
        && lhs.endpoint == rhs.endpoint;
}

bool operator!=(ib::mw::EndpointAddress lhs, ib::mw::EndpointAddress rhs)
{
    return lhs.participant != rhs.participant
        || lhs.endpoint != rhs.endpoint;
}

} // namespace mw
} // namespace ib
