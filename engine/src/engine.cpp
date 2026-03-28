#include "calliope/engine.h"
#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <cassert>

namespace calliope {

// --- Singleton ---

Engine& Engine::getInstance()
{
    static Engine instance;
    return instance;
}

Engine::~Engine()
{
    if (isInitialised())
        shutdown();
}

// --- Lifecycle ---

bool Engine::initialise(double sampleRate, int bufferSize)
{
    audioGraph_ = std::make_unique<AudioGraph>();
    return audioGraph_->initialise(sampleRate, bufferSize);
}

void Engine::shutdown()
{
    if (audioGraph_) {
        audioGraph_->shutdown();
        audioGraph_.reset();
    }
}

bool Engine::isInitialised() const
{
    return audioGraph_ && audioGraph_->isInitialised();
}

// --- Sub-component access ---

AudioGraph& Engine::getAudioGraph()
{
    assert(audioGraph_ && "Engine not initialised");
    return *audioGraph_;
}

Transport& Engine::getTransport()
{
    assert(audioGraph_ && "Engine not initialised");
    return audioGraph_->getTransport();
}

// --- Convenience wrappers ---

void Engine::transportPlay()    { getTransport().play(); }
void Engine::transportStop()    { getTransport().stop(); }
void Engine::transportPause()   { getTransport().pause(); }
void Engine::setBpm(double bpm) { getTransport().setBpm(bpm); }

void Engine::setTimeSignature(int numerator, int denominator)
{
    getTransport().setTimeSignature(numerator, denominator);
}

void Engine::setLoopRegion(double startBeat, double endBeat, bool enabled)
{
    getTransport().setLoopRegion(startBeat, endBeat, enabled);
}

bool Engine::setBufferSize(int bufferSize)
{
    if (!audioGraph_) return false;
    return audioGraph_->setBufferSize(bufferSize);
}

void Engine::setMetronomeEnabled(bool enabled)
{
    getAudioGraph().getMetronome().enabled.store(enabled, std::memory_order_relaxed);
}

void Engine::setMetronomeVolume(float volume)
{
    getAudioGraph().getMetronome().volume.store(volume, std::memory_order_relaxed);
}

// --- State queries ---

Engine::TransportStateInfo Engine::getTransportState() const
{
    assert(audioGraph_ && "Engine not initialised");
    auto& t = audioGraph_->getTransport();

    TransportStateInfo info;
    switch (t.getState()) {
        case TransportState::Stopped: info.state = "stopped"; break;
        case TransportState::Playing: info.state = "playing"; break;
        case TransportState::Paused:  info.state = "paused";  break;
    }
    info.bpm = t.getBpm();
    info.timeSigNumerator = t.getTimeSignatureNumerator();
    info.timeSigDenominator = t.getTimeSignatureDenominator();
    info.samplePosition = t.getSamplePosition();
    info.ppqPosition = t.getPpqPosition();
    info.looping = t.isLooping();
    info.loopStartBeat = t.getLoopStartBeat();
    info.loopEndBeat = t.getLoopEndBeat();

    return info;
}

Engine::AudioConfigInfo Engine::getAudioConfig() const
{
    AudioConfigInfo info;
    if (audioGraph_) {
        auto cfg = audioGraph_->getAudioConfig();
        info.sampleRate = cfg.sampleRate;
        info.bufferSize = cfg.bufferSize;
        info.initialised = true;
    } else {
        info.sampleRate = 0.0;
        info.bufferSize = 0;
        info.initialised = false;
    }
    return info;
}

// --- Static methods (preserved from Phase 1) ---

std::string Engine::getJuceVersion()
{
    return juce::SystemStats::getJUCEVersion().toStdString();
}

std::vector<std::string> Engine::getAudioDevices()
{
    juce::MessageManager::getInstance();

    juce::AudioDeviceManager manager;
    std::vector<std::string> devices;

    for (auto* type : manager.getAvailableDeviceTypes()) {
        type->scanForDevices();
        for (auto& name : type->getDeviceNames()) {
            devices.push_back(type->getTypeName().toStdString()
                + ": " + name.toStdString());
        }
    }

    return devices;
}

} // namespace calliope
