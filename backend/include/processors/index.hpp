#pragma once
#include <string>
#include <vector>
#include "vad.processor/vad.processor.hpp"
#include "voice.processor/voice.processor.hpp"
#include "audio.processor/audio.processor.hpp"
#include "audio.processor/vad.audio.processor.hpp"
#include "summary.processor/summary.processor.hpp"

namespace lekhanai
{
    class Processor
    {
    public:
        Processor();
        std::vector<std::string> process(const std::string &raw_audio);
        ~Processor();

    private:
        AudioProcessor *audio_processor;
        VoiceProcessor *voice_processor;
        VADProcessor *vad_processor;
        VADAudioProcessor *vad_audio_processor;
        SummaryProcessor *summary_processor;
        std::vector<float> getDecodedAudio(const std::string &raw_audio);
        std::string getVoiceTranscription(const std::vector<std::vector<float>> &audio);
        std::vector<SpeechSegment> getSpeechSegments(const std::vector<float> &audio);
        std::string getSummary(const std::string &transcription);
    };
}