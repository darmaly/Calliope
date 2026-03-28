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

// --- Instrument access ---

PolySynthProcessor& Engine::getPolySynth() { return getAudioGraph().getPolySynth(); }
BassSynthProcessor& Engine::getBassSynth() { return getAudioGraph().getBassSynth(); }
DrumMachineProcessor& Engine::getDrumMachine() { return getAudioGraph().getDrumMachine(); }

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

    // PolySynth parameters
    auto& ps = getAudioGraph().getPolySynth();
    paramRegistry_.registerParameter("polysynth.osc1Waveform", {
        [&ps]() -> juce::var { return ps.osc1Waveform.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.osc1Waveform.store(static_cast<int>(v)); },
        "int", 0, 3
    });
    paramRegistry_.registerParameter("polysynth.osc2Waveform", {
        [&ps]() -> juce::var { return ps.osc2Waveform.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.osc2Waveform.store(static_cast<int>(v)); },
        "int", 0, 3
    });
    paramRegistry_.registerParameter("polysynth.oscMix", {
        [&ps]() -> juce::var { return ps.oscMix.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.oscMix.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.0, 1.0
    });
    paramRegistry_.registerParameter("polysynth.osc2Detune", {
        [&ps]() -> juce::var { return ps.osc2Detune.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.osc2Detune.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", -100.0, 100.0
    });
    paramRegistry_.registerParameter("polysynth.filterCutoff", {
        [&ps]() -> juce::var { return ps.filterCutoff.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.filterCutoff.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 20.0, 20000.0
    });
    paramRegistry_.registerParameter("polysynth.filterResonance", {
        [&ps]() -> juce::var { return ps.filterResonance.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.filterResonance.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.0, 1.0
    });
    paramRegistry_.registerParameter("polysynth.filterEnvAmount", {
        [&ps]() -> juce::var { return ps.filterEnvAmount.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.filterEnvAmount.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.0, 1.0
    });
    paramRegistry_.registerParameter("polysynth.ampAttack", {
        [&ps]() -> juce::var { return ps.ampAttack.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.ampAttack.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.001, 10.0
    });
    paramRegistry_.registerParameter("polysynth.ampDecay", {
        [&ps]() -> juce::var { return ps.ampDecay.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.ampDecay.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.001, 10.0
    });
    paramRegistry_.registerParameter("polysynth.ampSustain", {
        [&ps]() -> juce::var { return ps.ampSustain.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.ampSustain.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.0, 1.0
    });
    paramRegistry_.registerParameter("polysynth.ampRelease", {
        [&ps]() -> juce::var { return ps.ampRelease.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.ampRelease.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.001, 10.0
    });
    paramRegistry_.registerParameter("polysynth.filterAttack", {
        [&ps]() -> juce::var { return ps.filterAttack.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.filterAttack.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.001, 10.0
    });
    paramRegistry_.registerParameter("polysynth.filterDecay", {
        [&ps]() -> juce::var { return ps.filterDecay.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.filterDecay.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.001, 10.0
    });
    paramRegistry_.registerParameter("polysynth.filterSustain", {
        [&ps]() -> juce::var { return ps.filterSustain.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.filterSustain.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.0, 1.0
    });
    paramRegistry_.registerParameter("polysynth.filterRelease", {
        [&ps]() -> juce::var { return ps.filterRelease.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.filterRelease.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.001, 10.0
    });
    paramRegistry_.registerParameter("polysynth.lfoRate", {
        [&ps]() -> juce::var { return ps.lfoRate.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.lfoRate.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.1, 20.0
    });
    paramRegistry_.registerParameter("polysynth.lfoDepth", {
        [&ps]() -> juce::var { return ps.lfoDepth.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.lfoDepth.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.0, 1.0
    });
    paramRegistry_.registerParameter("polysynth.lfoTarget", {
        [&ps]() -> juce::var { return ps.lfoTarget.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.lfoTarget.store(static_cast<int>(v)); },
        "int", 0, 2
    });
    paramRegistry_.registerParameter("polysynth.masterGain", {
        [&ps]() -> juce::var { return ps.masterGain.load(std::memory_order_relaxed); },
        [&ps](const juce::var& v) { ps.masterGain.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.0, 1.0
    });

    // BassSynth parameters
    auto& bs = getAudioGraph().getBassSynth();
    paramRegistry_.registerParameter("basssynth.oscWaveform", {
        [&bs]() -> juce::var { return bs.oscWaveform.load(std::memory_order_relaxed); },
        [&bs](const juce::var& v) { bs.oscWaveform.store(static_cast<int>(v)); },
        "int", 0, 3
    });
    paramRegistry_.registerParameter("basssynth.subOscMix", {
        [&bs]() -> juce::var { return bs.subOscMix.load(std::memory_order_relaxed); },
        [&bs](const juce::var& v) { bs.subOscMix.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.0, 1.0
    });
    paramRegistry_.registerParameter("basssynth.subOscOctave", {
        [&bs]() -> juce::var { return bs.subOscOctave.load(std::memory_order_relaxed); },
        [&bs](const juce::var& v) { bs.subOscOctave.store(static_cast<int>(v)); },
        "int", -2, -1
    });
    paramRegistry_.registerParameter("basssynth.filterCutoff", {
        [&bs]() -> juce::var { return bs.filterCutoff.load(std::memory_order_relaxed); },
        [&bs](const juce::var& v) { bs.filterCutoff.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 20.0, 5000.0
    });
    paramRegistry_.registerParameter("basssynth.filterResonance", {
        [&bs]() -> juce::var { return bs.filterResonance.load(std::memory_order_relaxed); },
        [&bs](const juce::var& v) { bs.filterResonance.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.0, 1.0
    });
    paramRegistry_.registerParameter("basssynth.filterEnvAmount", {
        [&bs]() -> juce::var { return bs.filterEnvAmount.load(std::memory_order_relaxed); },
        [&bs](const juce::var& v) { bs.filterEnvAmount.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.0, 1.0
    });
    paramRegistry_.registerParameter("basssynth.ampAttack", {
        [&bs]() -> juce::var { return bs.ampAttack.load(std::memory_order_relaxed); },
        [&bs](const juce::var& v) { bs.ampAttack.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.001, 10.0
    });
    paramRegistry_.registerParameter("basssynth.ampDecay", {
        [&bs]() -> juce::var { return bs.ampDecay.load(std::memory_order_relaxed); },
        [&bs](const juce::var& v) { bs.ampDecay.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.001, 10.0
    });
    paramRegistry_.registerParameter("basssynth.ampSustain", {
        [&bs]() -> juce::var { return bs.ampSustain.load(std::memory_order_relaxed); },
        [&bs](const juce::var& v) { bs.ampSustain.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.0, 1.0
    });
    paramRegistry_.registerParameter("basssynth.ampRelease", {
        [&bs]() -> juce::var { return bs.ampRelease.load(std::memory_order_relaxed); },
        [&bs](const juce::var& v) { bs.ampRelease.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.001, 10.0
    });
    paramRegistry_.registerParameter("basssynth.filterAttack", {
        [&bs]() -> juce::var { return bs.filterAttack.load(std::memory_order_relaxed); },
        [&bs](const juce::var& v) { bs.filterAttack.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.001, 10.0
    });
    paramRegistry_.registerParameter("basssynth.filterDecay", {
        [&bs]() -> juce::var { return bs.filterDecay.load(std::memory_order_relaxed); },
        [&bs](const juce::var& v) { bs.filterDecay.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.001, 10.0
    });
    paramRegistry_.registerParameter("basssynth.filterSustain", {
        [&bs]() -> juce::var { return bs.filterSustain.load(std::memory_order_relaxed); },
        [&bs](const juce::var& v) { bs.filterSustain.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.0, 1.0
    });
    paramRegistry_.registerParameter("basssynth.filterRelease", {
        [&bs]() -> juce::var { return bs.filterRelease.load(std::memory_order_relaxed); },
        [&bs](const juce::var& v) { bs.filterRelease.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.001, 10.0
    });
    paramRegistry_.registerParameter("basssynth.masterGain", {
        [&bs]() -> juce::var { return bs.masterGain.load(std::memory_order_relaxed); },
        [&bs](const juce::var& v) { bs.masterGain.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
        "float", 0.0, 1.0
    });

    // DrumMachine parameters
    auto& dm = getAudioGraph().getDrumMachine();
    paramRegistry_.registerParameter("drumMachine.volume", {
        [&dm]() -> juce::var { return dm.volume.load(std::memory_order_relaxed); },
        [&dm](const juce::var& v) { dm.volume.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
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
