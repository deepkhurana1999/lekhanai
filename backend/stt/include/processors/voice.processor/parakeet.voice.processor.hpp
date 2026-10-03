#pragma once
#include <parakeet.h>
#include <string>
#include <vector>
#include <mutex>
#include "voice.processor.hpp"

namespace lekhanai
{
    class ParakeetVoiceProcessor : public VoiceProcessor
    {
    public:
        explicit ParakeetVoiceProcessor(const std::string &model_path, int n_threads, int n_processors);
        ~ParakeetVoiceProcessor();

        std::string process(const std::vector<std::vector<float>> &batched_audio) override;

    private:
        parakeet_context *parakeet_ctx;
    };
}