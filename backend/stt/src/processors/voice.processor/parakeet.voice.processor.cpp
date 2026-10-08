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
        parakeet_context_params ctx_params = parakeet_context_default_params();
        parakeet_ctx = parakeet_init_from_file_with_params(model_path.c_str(), ctx_params);
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

        // Parakeet (unlike this project's forked whisper.cpp) has no
        // batch-parallel entry point, and running multiple segments
        // concurrently against one GPU context isn't actually faster anyway
        // (see the N_PROCESSORS=1 fix for the Whisper path) — process each
        // segment sequentially through the context's own persistent state.
        auto params = parakeet_full_default_params(PARAKEET_SAMPLING_GREEDY);
        params.n_threads = n_threads;

        std::string result = "";
        for (const auto &segment : batched_audio)
        {
            if (parakeet_full(parakeet_ctx, params, segment.data(), static_cast<int>(segment.size())) != 0)
            {
                continue; // Transcription failed for this segment; skip it
            }

            int n_segments = parakeet_full_n_segments(parakeet_ctx);
            for (int i = 0; i < n_segments; ++i)
            {
                result += parakeet_full_get_segment_text(parakeet_ctx, i);
                result += " ";
            }
        }

        if (result.empty())
        {
            return "[BLANK_AUDIO]";
        }
        return result;
    }
}