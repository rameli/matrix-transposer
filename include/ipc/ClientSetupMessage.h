#pragma

#include <cstdint>
#include <stddef.h>

struct ClientSetupMessage
{
    uint32_t clientID;
    size_t m;
    size_t n;
    size_t k;
};
