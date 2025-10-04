#pragma once
#include <string>
#include <vector>

namespace lekhanai
{
    class AudioProcessor
    {
    public:
        std::vector<float> process(const std::string &audioData);
    };
}