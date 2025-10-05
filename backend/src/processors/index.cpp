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
        const float max_speech_duration_s = std::numeric_limits<float>::infinity();
        const int min_speech_samples = sr_per_ms * min_speech_duration_ms;
        const int max_speech_samples = (sample_rate * max_speech_duration_s - window_size_samples - 2 * speech_pad_samples);
        const int min_silence_samples = sr_per_ms * min_silence_duration_ms;
        const int min_silence_samples_at_max_speech = sr_per_ms * 98;

        // Initialize processors
        const int n_threads = 4;
        const int n_processors = 1;

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

    std::vector<float> Processor::getDecodedAudio(const std::string &audioData)
    {
        return audio_processor->process(audioData);
    }

    std::string Processor::getVoiceTranscription(const std::vector<float> &pcmData)
    {
        return voice_processor->process(pcmData);
    }

    std::vector<SpeechSegment> Processor::getSpeechSegments(const std::vector<float> &pcmData)
    {
        return vad_audio_processor->process(pcmData);
    }

    Processor::~Processor()
    {
        delete audio_processor;
        delete voice_processor;
        delete vad_processor;
        delete vad_audio_processor;
    }
}