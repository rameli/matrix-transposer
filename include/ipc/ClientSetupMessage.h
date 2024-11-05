#pragma once

#include <cstdint>
#include <stddef.h>

/**
 * @brief Represents a setup message for a client.
 *
 * This structure contains configuration parameters necessary for
 * setting up the client, including identifiers and dimensions
 * relevant to the client's operation.
 */
struct ClientSetupMessage
{
    /**
     * @brief The unique identifier for the client.
     *
     * This field serves as a unique identifier for the client,
     * allowing the system to distinguish between different clients. Based on this client ID, the server identifies the client shared memory objects and other resources.
     * The client ID is unique across all clients and is used in real-time to process the requests.
     */
    uint32_t clientID;

    /**
     * @brief The first dimension parameter.
     *
     * The number of row of the client matrices is 2**m.
     */
    uint32_t m;

    /**
     * @brief The second dimension parameter.
     *
     * The number of columns of the client matrices is 2**n.
     */
    uint32_t n;

    /**
     * @brief The third dimension parameter.
     *
     * Number of matrix buffers provided by the client.
     */
    uint32_t k;
};
