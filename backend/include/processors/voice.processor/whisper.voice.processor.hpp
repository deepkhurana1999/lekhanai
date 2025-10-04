#pragma once
#include <whisper.h>
#include <string>
#include <vector>
#include <mutex>
#include "voice.processor.hpp"

namespace lekhanai
{
    class WhisperVoiceProcessor : public VoiceProcessor
    {
    public:
        explicit WhisperVoiceProcessor(const std::string &model_path);
        ~WhisperVoiceProcessor();

        std::string process(const std::vector<float> &pcmData) override;

    private:
        whisper_context *whisperCtx;
    };
}