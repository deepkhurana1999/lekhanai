#pragma once
#include <string>
#include "processors/vad.processor/vad.processor.hpp"

namespace lekhanai
{
    class VADProcessorFactory
    {
    public:
        VADProcessorFactory();
        VADProcessor *create(const std::string &model_path,
                             const std::string &type,
                             int windows_frame_size,
                             int sample_rate,
                             int effective_window_size);
    };
}