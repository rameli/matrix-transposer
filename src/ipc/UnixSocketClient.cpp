#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdexcept>

#include "ipc/UnixSocketClient.h"
#include "ipc/ClientSetupMessage.h"

UnixSocketClient::UnixSocketClient(uint32_t clientID, uint32_t m, uint32_t n, uint32_t k) :
    m_SocketPath {"/tmp/matrix_transposer_server.sock"},
    m_ClientSocket(-1)
{
    // Create the socket
    m_ClientSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_ClientSocket < 0) {
        exit(EXIT_FAILURE);
    }

    sockaddr_un servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sun_family = AF_UNIX;
    strncpy(servAddr.sun_path, m_SocketPath.c_str(), sizeof(servAddr.sun_path) - 1);

    if (connect(m_ClientSocket, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        throw std::runtime_error("Failed to connect to the server");
    }

    ClientSetupMessage setupMessage
    {
        .clientID = clientID,
        .m = m,
        .n = n,
        .k = k
    };

    send(m_ClientSocket, &setupMessage, sizeof(setupMessage), 0);
}

UnixSocketClient::~UnixSocketClient() {
    if (m_ClientSocket >= 0) {
        close(m_ClientSocket);
        m_ClientSocket = -1;
    }
}