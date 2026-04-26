#pragma once

#include <cstdint>
#include <string>

namespace bcos::tool
{
class TarsRPCConfig
{
public:
    std::string host;
    uint16_t port = 0;
    uint32_t threadCount = 0;
};
}  // namespace bcos::tool