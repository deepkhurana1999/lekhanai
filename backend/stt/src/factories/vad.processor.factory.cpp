#include <stdexcept>
#include <string>
#include "constants.hpp"
#include "factories/vad.processor.factory.hpp"
#include "processors/vad.processor/silero.vad.processor.hpp"

namespace lekhanai
{
    VADProcessorFactory::VADProcessorFactory() = default;
    VADProcessor *VADProcessorFactory::create(const std::string &model_path,
                                              const VAD_MODEL type,
                                              int windows_frame_size,
                                              int sample_rate,
                                              int effective_window_size)
    {
        if (type == VAD_MODEL::SILERO)
        {
            return new SileroVADProcessor(model_path, windows_frame_size, sample_rate, effective_window_size);
        }

        throw std::invalid_argument("Unsupported vad processor type: " + std::to_string(static_cast<int>(type)));
    }
}