#pragma once
#include <string>
#include "constants.hpp"
#include "processors/vad.processor/vad.processor.hpp"

namespace lekhanai
{
    class VADProcessorFactory
    {
    public:
        VADProcessorFactory();
        VADProcessor *create(const std::string &model_path,
                             const VAD_MODEL type,
                             int windows_frame_size,
                             int sample_rate,
                             int effective_window_size);
    };
}