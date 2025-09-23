#pragma once
#include <string>
#include <vector>

namespace lekhanai
{
    class AudioProcessor
    {
    public:
        std::vector<float> decode(const std::string &audioData);
    };
}