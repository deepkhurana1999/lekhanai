#pragma once
#include <string>
#include <vector>

namespace lekhanai
{
    class VADProcessor
    {
    public:
        VADProcessor(const std::string &model_path,
                     int windows_frame_size,
                     int sample_rate,
                     int effective_window_size);
        virtual float process(std::vector<float> &chunk, std::vector<float> &state) = 0;
        virtual void reset_states() = 0;

    protected:
        std::string model_path;
        int sample_rate;
        int windows_frame_size;
        int effective_window_size;
    };
}