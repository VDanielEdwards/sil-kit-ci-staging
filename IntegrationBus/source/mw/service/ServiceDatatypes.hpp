// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once

#include <vector>
#include <map>
#include <sstream>

#include "IIbEndpoint.hpp" //for ServiceId

namespace ib {
namespace mw {
namespace service {

struct ServiceDiscoveryEvent
{
    enum class Type
    {
        Invalid,
        ServiceCreated,
        ServiceRemoved,
    };
    Type type{ Type::Invalid };
    ServiceDescriptor service;
};

struct ServiceAnnouncement //requires history >= 1
{
    std::string participantName;
    uint64_t version{1}; //!< version indicator is manually set and changed when announcements break compatibility
    std::vector<ServiceDescriptor> services; //!< list of services provided by the participant
};

////////////////////////////////////////////////////////////////////////////////
// Inline operators
////////////////////////////////////////////////////////////////////////////////
inline bool operator==(const ServiceDiscoveryEvent& lhs, const ServiceDiscoveryEvent& rhs)
{
    return lhs.type == rhs.type
        && lhs.service == rhs.service
        ;
}
inline bool operator==(const ServiceAnnouncement& lhs, const ServiceAnnouncement& rhs)
{
    return lhs.participantName == rhs.participantName
        && lhs.services == rhs.services
        ;
}
inline bool operator!=(const ServiceAnnouncement& lhs, const ServiceAnnouncement& rhs)
{
    return !(lhs == rhs);
}
////////////////////////////////////////////////////////////////////////////////
// Inline string utils
////////////////////////////////////////////////////////////////////////////////

inline std::ostream& operator<<(std::ostream& out, const ServiceDiscoveryEvent::Type& t)
{
    switch (t)
    {
    case ServiceDiscoveryEvent::Type::Invalid: out << "Invalid"; break;
    case ServiceDiscoveryEvent::Type::ServiceCreated: out << "ServiceCreated"; break;
    case ServiceDiscoveryEvent::Type::ServiceRemoved: out << "ServiceRemoved"; break;
    default:
        out << "Unknown ServiceDiscoveryEvent::Type{"
            << static_cast<std::underlying_type_t<ServiceDiscoveryEvent::Type>>(t);
    }
    return out;
}
inline std::ostream& operator<<(std::ostream& out, const ServiceDiscoveryEvent& event)
{
    out << "ServiceDiscoveryEvent{" << event.type << ", " << event.service << "}";
    return out;
}

inline std::ostream& operator<<(std::ostream& out, const ServiceAnnouncement& serviceAnnouncement)
{
    out << "ServiceAnnouncement{\"" << serviceAnnouncement.participantName
        << "\" services=["
        ;
    for (auto&& service : serviceAnnouncement.services)
    {
        out << "Service{" << service
            << "}"
            ;
    }
    out << "] }";
    return out;
}
inline std::string to_string(const ServiceAnnouncement& serviceAnnouncement)
{
    std::stringstream str;
    str << serviceAnnouncement;
    return str.str();
}
} // namespace service
} // namespace mw
} // namespace ib