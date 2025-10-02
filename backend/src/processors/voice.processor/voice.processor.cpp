#include <string>
#include "processors/voice.processor/voice.processor.hpp"

namespace lekhanai
{
    VoiceProcessor::VoiceProcessor(const std::string &mPath)
    {
        modelPath = mPath;
    }
}