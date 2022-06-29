// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include "SystemController.hpp"

#include <chrono>
#include <functional>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "ib/mw/sync/string_utils.hpp"

#include "MockParticipant.hpp"
#include "SyncDatatypeUtils.hpp"

namespace {

using namespace std::chrono_literals;

using namespace testing;

using namespace ib;
using namespace ib::mw;
using namespace ib::mw::sync;
using namespace ib::util;

using ::ib::mw::test::DummyParticipant;

class MockParticipant : public DummyParticipant
{
public:
    MOCK_METHOD2(SendIbMessage, void(const IIbServiceEndpoint*, const ParticipantCommand& msg));
    MOCK_METHOD2(SendIbMessage, void(const IIbServiceEndpoint*, const SystemCommand& msg));
};

class SystemControllerTest : public testing::Test
{
protected:
    SystemControllerTest()
        : controller{&participant}
    {
        controller.SetServiceDescriptor(descriptor);
    }


protected:
    // ----------------------------------------
    // Helper Methods

protected:
    // ----------------------------------------
    // Members
    EndpointAddress addr{103, 1026};
    ServiceDescriptor descriptor = from_endpointAddress(addr);

    MockParticipant participant;
    SystemController controller;
};

TEST_F(SystemControllerTest, configure_serviceDescriptor)
{
    EXPECT_EQ(controller.GetServiceDescriptor(), from_endpointAddress(addr));
}

TEST_F(SystemControllerTest, send_restart)
{
    std::string name = "Participant";
    auto nameHash = util::hash::Hash(name);
    ParticipantCommand cmd{ nameHash, ParticipantCommand::Kind::Restart};
    EXPECT_CALL(participant, SendIbMessage(&controller, cmd)).Times(1);
    controller.Restart(name);
}

TEST_F(SystemControllerTest, send_run)
{
    SystemCommand cmd{SystemCommand::Kind::Run};
    EXPECT_CALL(participant, SendIbMessage(&controller, cmd)).Times(1);
    controller.Run();
}
    
TEST_F(SystemControllerTest, send_stop)
{
    SystemCommand cmd{SystemCommand::Kind::Stop};
    EXPECT_CALL(participant, SendIbMessage(&controller, cmd)).Times(1);
    controller.Stop();
}
    
} // anonymous namespace
