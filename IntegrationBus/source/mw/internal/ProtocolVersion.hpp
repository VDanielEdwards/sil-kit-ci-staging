// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once
#include <tuple>
namespace ib {
namespace mw {

// The protocol version is directly tied to the MessageBuffer for backward compatibility in Ser/Des

struct ProtocolVersion
{
    uint16_t major;
    uint16_t minor;
};
// ================================================================================
//  Inline Implementations
// ================================================================================

inline constexpr auto CurrentProtocolVersion() -> ProtocolVersion
{
    return {3, 1};
}

inline bool operator==(const ProtocolVersion& lhs, const ProtocolVersion& rhs)
{
    return lhs.major == rhs.major
        && lhs.minor == rhs.minor
        ;
}
inline bool operator!=(const ProtocolVersion& lhs, const ProtocolVersion& rhs)
{
    return !(lhs == rhs);
}

} // namespace mw
} // namespace ib
