#pragma once
#include <string>
#include <vector>

namespace hikki
{
    class AudioProcessor
    {
    public:
        std::vector<float> decode(const std::string &audioData);
    };
}