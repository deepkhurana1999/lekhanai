#pragma once
#include "audio.processor.hpp"
#include "voice.processor.hpp"

namespace srotalekh
{
    class Processor
    {
    public:
        Processor();
        std::vector<float> handleAudioDecode(const std::string &audioData);
        std::string handleVoiceTranscribe(const std::vector<float> &pcmData);
        ~Processor();

    private:
        AudioProcessor *audioProcessor;
        VoiceProcessor *voiceProcessor;
    };
}