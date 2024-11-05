#pragma once

#include <cstdint>
#include <stddef.h>

/**
 * @brief Represents a request from a client.
 *
 * This structure holds information about a client's request,
 * including the unique identifier for the client and the
 * index of the matrix associated with the request.
 */
struct ClientRequest
{
    /**
     * @brief The unique identifier for the client.
     *
     * This field is used to differentiate between multiple
     * clients making requests. It is be unique for each
     * client.
     */
    uint32_t clientId;

    /**
     * @brief The index of the matrix associated with the request.
     *
     * This field indicates which matrix the client's request is referring to.
     */
    uint32_t matrixIndex;
};
