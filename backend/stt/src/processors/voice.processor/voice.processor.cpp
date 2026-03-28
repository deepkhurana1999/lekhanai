#include <string>
#include "processors/voice.processor/voice.processor.hpp"

namespace lekhanai
{
    VoiceProcessor::VoiceProcessor(const std::string &model_path, int n_threads, int n_processors)
        : model_path(model_path), n_threads(n_threads), n_processors(n_processors)
    {
    }
}