#include <stdexcept>
#include <string>
#include <vector>
#include <thread>
#include <whisper.h>
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

    std::string WhisperVoiceProcessor::process(const std::vector<std::vector<float>> &batched_audio)
    {
        std::lock_guard<std::mutex> lock(threadMutex);
        if (batched_audio.empty())
        {
            return "[BLANK_AUDIO]";
        }
        else if (batched_audio.size() > n_processors)
        {
            throw std::invalid_argument("The size of batched_audio must be equal to n_processors");
        }

        auto params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        params.language = "en";
        params.n_threads = n_threads;
        params.print_timestamps = false;

        std::vector<int> size_per_batch;
        std::vector<const float *> batch_ptrs;
        for (auto &batch : batched_audio)
        {
            batch_ptrs.push_back(batch.data());
            size_per_batch.push_back(batch.size());
        }

        if (whisper_full_batch_parallel(whisperCtx, params, batch_ptrs.data(), size_per_batch.data(), batched_audio.size(), n_processors) != 0)
        {
            return "[BLANK_AUDIO]"; // Transcription failed
        }

        std::string result = "";
        int n_segments = whisper_full_n_segments(whisperCtx);
        for (int i = 0; i < n_segments; ++i)
        {
            result += whisper_full_get_segment_text(whisperCtx, i);
        }
        return result;
    }
}