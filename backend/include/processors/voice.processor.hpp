#pragma once
#include <whisper.h>
#include <string>
#include <vector>
#include <mutex>

namespace hikki
{
    class VoiceProcessor
    {
    public:
        explicit VoiceProcessor(const std::string &modelPath);
        ~VoiceProcessor();

        // Transcribe PCM audio and return text
        std::string transcribe(const std::vector<float> &pcmData);

    private:
        whisper_context *whisperCtx;
        std::mutex whisperMutex; // Guard for thread safety if needed
    };
}