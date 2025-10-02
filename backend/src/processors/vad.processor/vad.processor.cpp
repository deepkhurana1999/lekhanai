#include <string>
#include "processors/vad.processor/vad.processor.hpp"

namespace lekhanai
{

    VADProcessor::VADProcessor(const std::string &model_path,
                               int windows_frame_size,
                               int sample_rate,
                               int effective_window_size) : model_path(model_path),
                                                            windows_frame_size(windows_frame_size),
                                                            sample_rate(sample_rate),
                                                            effective_window_size(effective_window_size)
    {
    }
}