#include "constants.hpp"
#include "config/index.hpp"
#include "processors/index.hpp"
#include "factories/vad.processor.factory.hpp"
#include "factories/voice.processor.factory.hpp"
#include "processors/audio.processor/audio.processor.hpp"
#include "processors/audio.processor/vad.audio.processor.hpp"

namespace lekhanai
{
    Processor::Processor()
    {
        Config config = Environment::getConfig();
        const int sample_rate = 16000; // Fixed sample rate for VAD and AudioProcessor
        const int windows_frame_size = 32;
        const int sr_per_ms = sample_rate / 1000;                       // e.g., 16000 / 1000 = 16
        const int window_size_samples = windows_frame_size * sr_per_ms; // e.g., 32ms * 16 = 512 samples
        const int min_silence_duration_ms = 100;
        const int speech_pad_samples = 30;
        const int min_speech_duration_ms = 250;
        const int context_samples = 64; // For 16kHz, 64 samples are added as context.
        const float voice_activation_threshold = 0.5;
        const int effective_window_size = window_size_samples + context_samples;
        const float max_speech_duration_s = 10.0f;
        const int min_speech_samples = sr_per_ms * min_speech_duration_ms;
        const int max_speech_samples = (sample_rate * max_speech_duration_s - window_size_samples - 2 * speech_pad_samples);
        const int min_silence_samples = sr_per_ms * min_silence_duration_ms;
        const int min_silence_samples_at_max_speech = sr_per_ms * 98;

        // Initialize processors
        const int n_threads = 4;
        const int n_processors = 4;

        audio_processor = new AudioProcessor();
        voice_processor = VoiceProcessorFactory().create(config.model_path, STT_MODEL::WHISPER, n_threads, n_processors);
        vad_processor = VADProcessorFactory().create(
            config.vad_model_path,
            VAD_MODEL::SILERO,
            windows_frame_size,
            sample_rate,
            effective_window_size);

        vad_audio_processor = new VADAudioProcessor(
            vad_processor,
            sample_rate,
            context_samples,
            window_size_samples,
            effective_window_size,
            min_silence_samples,
            min_silence_samples_at_max_speech,
            min_speech_samples,
            max_speech_samples,
            speech_pad_samples,
            voice_activation_threshold);
    }

    std::vector<float> Processor::getDecodedAudio(const std::string &raw_audio)
    {
        return audio_processor->process(raw_audio);
    }

    std::string Processor::getVoiceTranscription(const std::vector<std::vector<float>> &audio)
    {
        return voice_processor->process(audio);
    }

    std::vector<SpeechSegment> Processor::getSpeechSegments(const std::vector<float> &audio)
    {
        return vad_audio_processor->process(audio);
    }

    std::vector<std::string> Processor::process(const std::string &raw_audio)
    {
        std::vector<std::string> result;
        std::vector<float> audio_data = getDecodedAudio(raw_audio);
        auto speech_segments = getSpeechSegments(audio_data);
        if (speech_segments.empty())
        {
            result.push_back("[BLANK_AUDIO]");
            return result;
        }

        int n_processors = 4;
        int batch_number = -1;
        std::vector<std::vector<std::vector<float>>> batched_audio;
        for (int i = 0; i < speech_segments.size(); ++i)
        {
            // Check if we need a new batch
            if (batch_number == -1 ||
                batched_audio[batch_number].size() >= n_processors)
            {

                // Create new batch
                batched_audio.push_back(std::vector<std::vector<float>>());
                batch_number++;
            }

            std::vector<float> segmented_audio(audio_data.begin() + speech_segments[i].start, audio_data.begin() + speech_segments[i].end + 1);
            batched_audio[batch_number].push_back(segmented_audio);
        }

        for (const auto &batch : batched_audio)
        {
            std::string transcription = getVoiceTranscription(batch);
            result.push_back(transcription);
        }

        return result;
    }

    Processor::~Processor()
    {
        delete audio_processor;
        delete voice_processor;
        delete vad_processor;
        delete vad_audio_processor;
    }
}