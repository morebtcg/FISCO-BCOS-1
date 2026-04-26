#pragma once

namespace bcos::tool
{
class BaselineSchedulerConfig
{
public:
    bool parallel = false;
    int grainSize = 0;
    int maxThread = 0;
};
}  // namespace bcos::tool