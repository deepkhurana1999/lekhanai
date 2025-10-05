#include <stdexcept>
#include <string>
#include <vector>
#include "processors/voice.processor/voice.processor.hpp"
#include "processors/voice.processor/whisper.voice.processor.hpp"

namespace lekhanai
{
    WhisperVoiceProcessor::WhisperVoiceProcessor(const std::string &model_path, int n_threads, int n_processors) : VoiceProcessor(model_path, n_threads, n_processors)
    {
        whisperCtx = whisper_init_from_file(model_path.c_str());
        if (!whisperCtx)
        {
            throw std::runtime_error("Failed to load Whisper model at " + model_path);
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
        params.n_threads = n_threads;
        params.print_timestamps = false;

        if (whisper_full_parallel(whisperCtx, params, pcmData.data(), pcmData.size(), n_processors) != 0)
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