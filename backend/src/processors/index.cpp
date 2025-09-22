#include "config/index.hpp"
#include "processors/index.hpp"
#include "processors/audio.processor.hpp"
#include "processors/voice.processor.hpp"
namespace srotalekh
{
    Processor::Processor()
    {
        Config config = Environment::getConfig();
        audioProcessor = new AudioProcessor();
        voiceProcessor = new VoiceProcessor(config.modelPath);
    }

    std::vector<float> Processor::handleAudioDecode(const std::string &audioData)
    {
        return audioProcessor->decode(audioData);
    }

    std::string Processor::handleVoiceTranscribe(const std::vector<float> &pcmData)
    {
        return voiceProcessor->transcribe(pcmData);
    }

    Processor::~Processor()
    {
        delete audioProcessor;
        delete voiceProcessor;
    }
}