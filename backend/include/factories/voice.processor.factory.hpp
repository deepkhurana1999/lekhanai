#pragma once
#include <string>
#include "constants.hpp"
#include "processors/voice.processor/voice.processor.hpp"

namespace lekhanai
{
    class VoiceProcessorFactory
    {
    public:
        VoiceProcessorFactory();
        VoiceProcessor *create(const std::string &model_path, const STT_MODEL type);
    };
}