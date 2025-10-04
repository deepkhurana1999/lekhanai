#pragma once
#include <string>
#include <vector>
#include "vad.processor/vad.processor.hpp"
#include "voice.processor/voice.processor.hpp"
#include "audio.processor/audio.processor.hpp"
#include "audio.processor/vad.audio.processor.hpp"

namespace lekhanai
{
    class Processor
    {
    public:
        Processor();
        std::vector<float> getDecodedAudio(const std::string &audioData);
        std::string getVoiceTranscription(const std::vector<float> &pcmData);
        std::vector<SpeechSegment> getSpeechSegments(const std::vector<float> &pcmData);
        ~Processor();

    private:
        AudioProcessor *audio_processor;
        VoiceProcessor *voice_processor;
        VADProcessor *vad_processor;
        VADAudioProcessor *vad_audio_processor;
    };
}