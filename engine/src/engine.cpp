#include "calliope/engine.h"
#include "calliope/project_state.h"
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
    bool ok = audioGraph_->initialise(sampleRate, bufferSize);
    if (ok) {
        registerParameters();
    }
    return ok;
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

// --- Command dispatcher and parameter registry ---

CommandDispatcher& Engine::getCommandDispatcher()
{
    return dispatcher_;
}

ParameterRegistry& Engine::getParameterRegistry()
{
    return paramRegistry_;
}

ProjectState Engine::getProjectState() const
{
    ProjectState state;
    state.snapshotFromEngine(*this);
    return state;
}

void Engine::registerParameters()
{
    // Transport parameters
    paramRegistry_.registerParameter("transport.bpm", {
        [this]() -> juce::var { return getTransport().getBpm(); },
        [this](const juce::var& v) { getTransport().setBpm(static_cast<double>(v)); },
        "double", 20.0, 999.0
    });

    paramRegistry_.registerParameter("transport.timeSigNumerator", {
        [this]() -> juce::var { return getTransport().getTimeSignatureNumerator(); },
        [this](const juce::var& v) { getTransport().setTimeSignature(static_cast<int>(v), getTransport().getTimeSignatureDenominator()); },
        "int", 1, 32
    });

    paramRegistry_.registerParameter("transport.timeSigDenominator", {
        [this]() -> juce::var { return getTransport().getTimeSignatureDenominator(); },
        [this](const juce::var& v) { getTransport().setTimeSignature(getTransport().getTimeSignatureNumerator(), static_cast<int>(v)); },
        "int", 1, 32
    });

    // Metronome parameters
    paramRegistry_.registerParameter("metronome.enabled", {
        [this]() -> juce::var { return getAudioGraph().getMetronome().enabled.load(); },
        [this](const juce::var& v) { getAudioGraph().getMetronome().enabled.store(static_cast<bool>(v)); },
        "bool", false, true
    });

    paramRegistry_.registerParameter("metronome.volume", {
        [this]() -> juce::var { return getAudioGraph().getMetronome().volume.load(); },
        [this](const juce::var& v) { getAudioGraph().getMetronome().volume.store(static_cast<float>(static_cast<double>(v))); },
        "float", 0.0, 1.0
    });

    // Master bus parameters
    paramRegistry_.registerParameter("master.volume", {
        [this]() -> juce::var { return getAudioGraph().getMasterBus().masterVolume.load(); },
        [this](const juce::var& v) { getAudioGraph().getMasterBus().masterVolume.store(static_cast<float>(static_cast<double>(v))); },
        "float", 0.0, 2.0
    });
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
