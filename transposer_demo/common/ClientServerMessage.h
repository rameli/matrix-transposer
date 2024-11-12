#pragma once

#include <cstdint>
#include <stddef.h>
#include <string>
#include <sstream>


// alignas(1) is used to ensure that the struct is packed and has no padding
struct alignas(1) ClientServerMessage
{
    enum class MessageType : uint32_t
    {
        Subscribe,
        Unsubscribe,
        SubscribeResponse,
    };    

    MessageType type;
    uint32_t senderId;
    uint32_t param1;
    uint32_t param2;
    uint32_t param3;

    static bool ProcessSubscribeMessage(const ClientServerMessage& message, uint32_t& clientId, uint32_t& m, uint32_t& n, uint32_t& k)
    {
        if (message.type != MessageType::Subscribe)
        {
            return false;
        }

        clientId = message.senderId;
        m = message.param1;
        n = message.param2;
        k = message.param3;

        return true;
    }

    static void GenerateSubscribeMessage(ClientServerMessage& message, const uint32_t& clientId, const uint32_t& m, const uint32_t& n, const uint32_t& k)
    {
        message.type = MessageType::Subscribe;
        message.senderId = clientId;
        message.param1 = m;
        message.param2 = n;
        message.param3 = k;
    }

    static bool ProcessUnsubscribeMessage(const ClientServerMessage& message, uint32_t& clientId)
    {
        if (message.type != MessageType::Unsubscribe)
        {
            return false;
        }

        clientId = message.senderId;

        return true;
    }

    static void GenerateUnsubscribeMessage(ClientServerMessage& message, const uint32_t& clientId)
    {
        message.type = MessageType::Unsubscribe;
        message.senderId = clientId;
    }

    static bool ProcessSubscribeResponseMessage(const ClientServerMessage& message, uint32_t& serverId, uint32_t& serverSideClientId, uint32_t& serverCapacity, uint32_t& serverClientCount)
    {
        if (message.type != MessageType::SubscribeResponse)
        {
            return false;
        }

        serverId = message.senderId;
        serverSideClientId = message.param1;
        serverCapacity = message.param2;
        serverClientCount = message.param3;

        return true;
    }

    static void GenerateSubscribeResponseMessage(ClientServerMessage& message, const uint32_t& serverId, const uint32_t& serverSideClientId, const uint32_t& serverCapacity, const uint32_t& serverClientCount)
    {
        message.type = MessageType::SubscribeResponse;
        message.senderId = serverId;
        message.param1 = serverSideClientId;
        message.param2 = serverCapacity;
        message.param3 = serverClientCount;
    }

    static std::string TypeToString(ClientServerMessage::MessageType type)
    {
        switch (type)
        {
        case MessageType::Subscribe:
            return "Subscribe";
        case MessageType::Unsubscribe:
            return "Unsubscribe";
        case MessageType::SubscribeResponse:

        default:
            return "UNKNOWN";
        }
    }

    static std::string ToString(const ClientServerMessage& message)
    {
        std::ostringstream oss;
        
        switch (message.type)
        {
        case MessageType::Subscribe:
            oss << "Subscribe: { clientPid: " << message.senderId << ", m: " << message.param1 << ", n: " << message.param2 << ", k: " << message.param3 << " }";
            break;
        case MessageType::Unsubscribe:
            oss << "Unsubscribe: { clientPid: " << message.senderId << " }";
            break;
        case MessageType::SubscribeResponse:
            oss << "SubscribeResponse: { serverPid: " << message.senderId << ", serverSideClientId: " << message.param1 << ", serverCapacity: " << message.param2 << ", serverClientCount: " << message.param3 << " }";
            break;
        default:
            oss << "UNKNOWN";
            break;
        }

        return oss.str();     
    }
};
