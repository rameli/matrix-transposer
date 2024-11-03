#include <gtest/gtest.h>
#include <memory>

#include "UnixSocketServer.h"

TEST(UnixSocketsTestSuite, ServerInitTearDown)
{
    const std::string socketPath = "/tmp/transpose_server.sock";
    std::unique_ptr<UnixSocketServer> pServer;

    EXPECT_NO_THROW(pServer = std::make_unique<UnixSocketServer>(socketPath));
    // EXPECT_NO_THROW(pServer->stop());
}