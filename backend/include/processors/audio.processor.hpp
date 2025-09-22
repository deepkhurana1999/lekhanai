#pragma once
#include <string>
#include <vector>

namespace srotalekh
{
    class AudioProcessor
    {
    public:
        std::vector<float> decode(const std::string &audioData);
    };
}