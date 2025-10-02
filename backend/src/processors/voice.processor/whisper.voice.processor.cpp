#include <stdexcept>
#include <string>
#include <vector>
#include "processors/voice.processor/voice.processor.hpp"
#include "processors/voice.processor/whisper.voice.processor.hpp"

namespace lekhanai
{
    WhisperVoiceProcessor::WhisperVoiceProcessor(const std::string &modelPath) : VoiceProcessor(modelPath)
    {
        whisperCtx = whisper_init_from_file(modelPath.c_str());
        if (!whisperCtx)
        {
            throw std::runtime_error("Failed to load Whisper model at " + modelPath);
        }
    }

    WhisperVoiceProcessor::~WhisperVoiceProcessor()
    {
        if (whisperCtx)
        {
            whisper_free(whisperCtx);
            whisperCtx = nullptr;
        }
    }

    std::string WhisperVoiceProcessor::process(const std::vector<float> &pcmData)
    {
        std::lock_guard<std::mutex> lock(threadMutex);

        auto params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        params.language = "en";
        params.n_threads = 4;
        params.print_timestamps = false;

        if (whisper_full(whisperCtx, params, pcmData.data(), pcmData.size()) != 0)
        {
            return ""; // Transcription failed
        }

        std::string result;
        int nSegments = whisper_full_n_segments(whisperCtx);
        for (int i = 0; i < nSegments; ++i)
        {
            result += whisper_full_get_segment_text(whisperCtx, i);
        }
        return result;
    }
}