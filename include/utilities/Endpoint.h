/**
 * @file Endpoint.h
 * @brief Defines the Endpoint enumeration for specifying connection roles.
 */

#pragma once

/**
 * @enum Endpoint
 * @brief Represents the role of an endpoint in a network connection.
 * 
 * This enumeration is used to specify whether the endpoint is acting as a 
 * server or a client in a network communication context.
 */
enum class Endpoint
{
    SERVER, ///< The endpoint is acting as a server.
    CLIENT  ///< The endpoint is acting as a client.
};
