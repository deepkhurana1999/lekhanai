#include <stdexcept>
#include <string>
#include "factories/voice.processor.factory.hpp"
#include "processors/voice.processor/whisper.voice.processor.hpp"

namespace lekhanai
{
    VoiceProcessorFactory::VoiceProcessorFactory() = default;
    VoiceProcessor *VoiceProcessorFactory::create(const std::string &model_path, const std::string &type)
    {
        if (type == "whisper")
        {
            return new WhisperVoiceProcessor(model_path);
        }

        throw std::invalid_argument("Unsupported voice processor type: " + type);
    }
}