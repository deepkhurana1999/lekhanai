#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "constants.hpp"
#include "vad.processor/vad.processor.hpp"
#include "voice.processor/voice.processor.hpp"
#include "audio.processor/audio.processor.hpp"
#include "audio.processor/vad.audio.processor.hpp"
#include "summary.processor/summary.processor.hpp"

using json = nlohmann::json;
namespace lekhanai
{
    class RequestProcessor
    {
    public:
        RequestProcessor();
        json process(REQUEST_MESSAGE_TYPE request_message_type, const std::string &payload);
        ~RequestProcessor();

    private:
        AudioProcessor *audio_processor;
        VoiceProcessor *voice_processor;
        VADProcessor *vad_processor;
        VADAudioProcessor *vad_audio_processor;
        SummaryProcessor *summary_processor;

        std::string getSummary(const std::string &transcription);
        std::vector<float> getDecodedAudio(const std::string &raw_audio);
        std::string getVoiceTranscription(const std::vector<std::vector<float>> &audio);
        std::vector<SpeechSegment> getSpeechSegments(const std::vector<float> &audio);
        std::vector<std::string> getVoiceTranscriptions(const std::string &raw_audio);
    };
}