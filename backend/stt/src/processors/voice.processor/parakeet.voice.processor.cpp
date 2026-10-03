#include <stdexcept>
#include <string>
#include <vector>
#include <thread>
#include <parakeet.h>
#include "processors/voice.processor/voice.processor.hpp"
#include "processors/voice.processor/parakeet.voice.processor.hpp"

namespace lekhanai
{
    ParakeetVoiceProcessor::ParakeetVoiceProcessor(const std::string &model_path, int n_threads, int n_processors) : VoiceProcessor(model_path, n_threads, n_processors)
    {
        parakeet_ctx = parakeet_init_from_file(model_path.c_str());
        if (!parakeet_ctx)
        {
            throw std::runtime_error("Failed to load Parakeet model at " + model_path);
        }
    }

    ParakeetVoiceProcessor::~ParakeetVoiceProcessor()
    {
        if (parakeet_ctx)
        {
            parakeet_free(parakeet_ctx);
            parakeet_ctx = nullptr;
        }
    }

    std::string ParakeetVoiceProcessor::process(const std::vector<std::vector<float>> &batched_audio)
    {
        std::lock_guard<std::mutex> lock(thread_mutex);
        if (batched_audio.empty())
        {
            return "[BLANK_AUDIO]";
        }
        else if (batched_audio.size() > n_processors)
        {
            throw std::invalid_argument("The size of batched_audio must be equal to n_processors");
        }

        auto params = parakeet_full_default_params(PARAKEET_SAMPLING_GREEDY);
        params.language = "en";
        params.n_threads = n_threads;
        params.print_timestamps = false;

        // 1. Calculate the total size required to avoid multiple reallocations
        size_t total_samples = 0;
        for (const auto& batch : batched_audio) {
            total_samples += batch.size();
        }

        // 2. Flatten the matrix into a single continuous memory block
        std::vector<float> continuous_audio;
        continuous_audio.reserve(total_samples);
        for (const auto& batch : batched_audio) {
            continuous_audio.insert(continuous_audio.end(), batch.begin(), batch.end());
        }
        
        if (parakeet_full(pctx, params, continuous_audio.data(), continuous_audio.size()) != 0)
        {
            return "[BLANK_AUDIO]"; // Transcription failed
        }

        std::string result = "";
        int n_segments = parakeet_full_n_segments(parakeet_ctx);
        for (int i = 0; i < n_segments; ++i)
        {
            result += parakeet_full_get_segment_text(parakeet_ctx, i);
            result += " "; 
        }
        return result;
    }
}