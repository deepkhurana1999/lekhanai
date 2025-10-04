#include <string>
#include <memory>
#include <onnxruntime_cxx_api.h>
#include "processors/vad.processor/vad.processor.hpp"
#include "processors/audio.processor/vad.audio.processor.hpp"

namespace lekhanai
{
    VADAudioProcessor::VADAudioProcessor(
        VADProcessor *voice_detection_processor,
        int sample_rate,
        int context_samples,
        int window_size_samples,
        int effective_window_size,
        int min_silence_samples,
        int min_silence_samples_at_max_speech,
        int min_speech_samples,
        float max_speech_samples,
        int speech_pad_samples,
        float voice_activation_threshold) : sample_rate(sample_rate),
                                            context_samples(context_samples),
                                            window_size_samples(window_size_samples),
                                            effective_window_size(effective_window_size),
                                            min_silence_samples(min_silence_samples),
                                            min_silence_samples_at_max_speech(min_silence_samples_at_max_speech),
                                            min_speech_samples(min_speech_samples),
                                            max_speech_samples(max_speech_samples),
                                            speech_pad_samples(speech_pad_samples),
                                            voice_activation_threshold(voice_activation_threshold)
    {
        vad_processor = voice_detection_processor;
        context.assign(context_samples, 0.0f);
        state.resize(size_state);
    }

    void VADAudioProcessor::reset_states()
    {
        std::memset(state.data(), 0, state.size() * sizeof(float));
        triggered = false;
        temp_end = 0;
        current_sample = 0;
        prev_end = next_start = 0;
        speeches.clear();
        current_speech = SpeechSegment();
        std::fill(context.begin(), context.end(), 0.0f);
    }

    std::vector<SpeechSegment> VADAudioProcessor::process(const std::vector<float> &audio)
    {
        reset_states();
        const float threshold = voice_activation_threshold;
        const int audio_length_samples = static_cast<int>(audio.size());
        // Process audio in chunks of window_size_samples (e.g., 512 samples)
        for (size_t j = 0; j < static_cast<size_t>(audio_length_samples); j += static_cast<size_t>(window_size_samples))
        {
            if (j + static_cast<size_t>(window_size_samples) > static_cast<size_t>(audio_length_samples))
                break;
            std::vector<float> chunk(&audio[j], &audio[j] + window_size_samples);

            // Build new input: first context_samples from context, followed by the current chunk (window_size_samples).
            std::vector<float> chunk_with_context(effective_window_size, 0.0f);
            std::copy(context.begin(), context.end(), chunk_with_context.begin());
            std::copy(chunk.begin(), chunk.end(), chunk_with_context.begin() + context_samples);

            // note: process the chunk with context to get speech probability
            // the state is updated within the vad_processor
            float speech_prob = vad_processor->process(chunk_with_context, state);
            current_sample += static_cast<unsigned int>(window_size_samples); // Advance by the original window size.

            // If speech is detected (probability >= threshold)
            if (speech_prob >= threshold)
            {
                if (temp_end != 0)
                {
                    temp_end = 0;
                    if (next_start < prev_end)
                        next_start = current_sample - window_size_samples;
                }
                if (!triggered)
                {
                    triggered = true;
                    current_speech.start = current_sample - window_size_samples;
                }
                // Update context: copy the last context_samples from chunk_with_context.
                std::copy(chunk_with_context.end() - context_samples, chunk_with_context.end(), context.begin());
                continue;
            }

            // If the speech segment becomes too long.
            if (triggered && ((current_sample - current_speech.start) > max_speech_samples))
            {
                if (prev_end > 0)
                {
                    current_speech.end = prev_end;
                    speeches.push_back(current_speech);
                    current_speech = SpeechSegment();
                    if (next_start < prev_end)
                        triggered = false;
                    else
                        current_speech.start = next_start;
                    prev_end = 0;
                    next_start = 0;
                    temp_end = 0;
                }
                else
                {
                    current_speech.end = current_sample;
                    speeches.push_back(current_speech);
                    current_speech = SpeechSegment();
                    prev_end = 0;
                    next_start = 0;
                    temp_end = 0;
                    triggered = false;
                }
                std::copy(chunk_with_context.end() - context_samples, chunk_with_context.end(), context.begin());
                continue;
            }

            if ((speech_prob >= (threshold - 0.15)) && (speech_prob < threshold))
            {
                // When the speech probability temporarily drops but is still in speech, update context without changing state.
                std::copy(chunk_with_context.end() - context_samples, chunk_with_context.end(), context.begin());
                continue;
            }

            if (speech_prob < (threshold - 0.15))
            {
                if (triggered)
                {
                    if (temp_end == 0)
                        temp_end = current_sample;
                    if (current_sample - temp_end > min_silence_samples_at_max_speech)
                        prev_end = temp_end;
                    if ((current_sample - temp_end) >= min_silence_samples)
                    {
                        current_speech.end = temp_end;
                        if (current_speech.end - current_speech.start > min_speech_samples)
                        {
                            speeches.push_back(current_speech);
                            current_speech = SpeechSegment();
                            prev_end = 0;
                            next_start = 0;
                            temp_end = 0;
                            triggered = false;
                        }
                    }
                }
                std::copy(chunk_with_context.end() - context_samples, chunk_with_context.end(), context.begin());
                continue;
            }
        }
        if (current_speech.start >= 0)
        {
            current_speech.end = audio_length_samples;
            speeches.push_back(current_speech);
            current_speech = SpeechSegment();
            prev_end = 0;
            next_start = 0;
            temp_end = 0;
            triggered = false;
        }
        return speeches;
    }
}