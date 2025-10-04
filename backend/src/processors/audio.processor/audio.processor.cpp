#include <vector>
#include <cstdint>
#include <cstring>
#include <string>
#include "processors/audio.processor/audio.processor.hpp"

using namespace std;
namespace lekhanai
{
    std::vector<float> AudioProcessor::process(const std::string &audioData)
    {
        // Each PCM sample is 2 bytes (int16_t)
        if (audioData.size() % 2 != 0)
        {
            // Invalid size
            return {};
        }

        size_t sample_count = audioData.size() / 2;
        const int16_t *pcm_data = reinterpret_cast<const int16_t *>(audioData.data());

        std::vector<float> float_data(sample_count);
        constexpr float normalization_factor = 1.0f / 32768.0f; // int16 range to [-1,1]

        for (size_t i = 0; i < sample_count; ++i)
        {
            // Convert int16 sample to float in range [-1, 1]
            float_data[i] = static_cast<float>(pcm_data[i]) * normalization_factor;
        }

        return float_data;
    }

}
