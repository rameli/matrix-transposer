#pragma

#include <cstdint>
#include <stddef.h>

struct ClientSetupMessage
{
    uint32_t clientID;
    uint32_t m;
    uint32_t n;
    uint32_t k;
};
