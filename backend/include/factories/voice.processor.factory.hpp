#pragma once
#include <string>
#include "processors/voice.processor/voice.processor.hpp"

namespace lekhanai
{
    class VoiceProcessorFactory
    {
    public:
        VoiceProcessorFactory();
        VoiceProcessor *create(const std::string &modelPath, const std::string &type);
    };
}