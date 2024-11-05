#include <gtest/gtest.h>
#include <memory>
#include <thread>

#include "ipc/UnixSocketServer.h"
#include "ipc/UnixSocketClient.h"

TEST(UnixSocketsTestSuite, ServerInitTearDown)
{
    std::unique_ptr<UnixSocketServer> pServer;

    EXPECT_NO_THROW(pServer = std::make_unique<UnixSocketServer>());
    EXPECT_NO_THROW(pServer->stop());
}

TEST(UnixSocketsTestSuite, SingleConnection)
{
    uint32_t clientID = 132;
    size_t m = 3;
    size_t n = 33;
    size_t k = 333;

    std::unique_ptr<UnixSocketServer> pServer;
    std::unique_ptr<UnixSocketClient> pClient;

    EXPECT_NO_THROW(pServer = std::make_unique<UnixSocketServer>());
    EXPECT_NO_THROW(pClient = std::make_unique<UnixSocketClient>(clientID, m, n, k));

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_NO_THROW(pServer.reset());
    EXPECT_NO_THROW(pClient.reset());
}