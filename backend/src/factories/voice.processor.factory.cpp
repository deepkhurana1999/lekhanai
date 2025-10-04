#include <stdexcept>
#include <string>
#include "constants.hpp"
#include "factories/voice.processor.factory.hpp"
#include "processors/voice.processor/whisper.voice.processor.hpp"

namespace lekhanai
{
    VoiceProcessorFactory::VoiceProcessorFactory() = default;
    VoiceProcessor *VoiceProcessorFactory::create(const std::string &model_path, const STT_MODEL type)
    {
        if (type == STT_MODEL::WHISPER)
        {
            return new WhisperVoiceProcessor(model_path);
        }

        throw std::invalid_argument("Unsupported voice processor type: " + type);
    }
}