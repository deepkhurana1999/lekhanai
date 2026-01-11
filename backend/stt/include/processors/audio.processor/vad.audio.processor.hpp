#pragma once
#include <vector>
#include <deque>
#include <string>
#include "processors/vad.processor/vad.processor.hpp"

namespace lekhanai
{
    class SpeechSegment
    {
    public:
        int start;
        int end;

        SpeechSegment(int start = -1, int end = -1) : start(start), end(end) {}

        SpeechSegment &operator=(const SpeechSegment &a)
        {
            start = a.start;
            end = a.end;
            return *this;
        }

        bool operator==(const SpeechSegment &a) const
        {
            return (start == a.start && end == a.end);
        }
    };

    class VADAudioProcessor
    {
    private:
        VADProcessor *vad_processor;
        int sample_rate;
        int window_size_samples;
        int min_silence_samples;
        int min_silence_samples_at_max_speech;
        int min_speech_samples;
        float max_speech_samples;
        int speech_pad_samples;
        float voice_activation_threshold;
        int effective_window_size;

        // ----- Context-related additions -----
        int context_samples;
        std::vector<float> context; // Holds the last 64 samples from the previous chunk (initialized to zero).

        // State management
        unsigned int size_state = 2 * 1 * 128;
        bool triggered = false;
        unsigned int temp_end = 0;
        unsigned int current_sample = 0;
        int prev_end;
        int next_start = 0;
        std::vector<float> state;
        std::vector<SpeechSegment> speeches;
        SpeechSegment current_speech;

        void reset_states();

    public:
        VADAudioProcessor(
            VADProcessor *vad_processor,
            int sample_rate,
            int context_samples,
            int window_size_samples,
            int effective_window_size,
            int min_silence_samples,
            int min_silence_samples_at_max_speech,
            int min_speech_samples,
            float max_speech_samples,
            int speech_pad_samples,
            float voice_activation_threshold);

        std::vector<SpeechSegment> process(const std::vector<float> &samples);
    };

}