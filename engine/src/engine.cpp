#include "calliope/engine.h"
#include "calliope/audio_exporter.h"
#include "calliope/project_state.h"
#include "calliope/effects/parametric_eq.h"
#include "calliope/effects/compressor.h"
#include "calliope/effects/reverb.h"
#include "calliope/effects/delay.h"
#include "calliope/effects/limiter.h"
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
        audioExporter_ = std::make_unique<AudioExporter>(*this);
        registerParameters();
    }
    return ok;
}

void Engine::shutdown()
{
    audioExporter_.reset();
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

// --- Audio exporter ---

AudioExporter& Engine::getAudioExporter()
{
    assert(audioExporter_ && "Engine not initialised");
    return *audioExporter_;
}

// --- Instrument access ---

PolySynthProcessor& Engine::getPolySynth() { return getAudioGraph().getPolySynth(); }
BassSynthProcessor& Engine::getBassSynth() { return getAudioGraph().getBassSynth(); }
DrumMachineProcessor& Engine::getDrumMachine() { return getAudioGraph().getDrumMachine(); }

InsertChain& Engine::getInsertChain(const juce::String& trackId)
{
    return getAudioGraph().getInsertChain(trackId);
}

AudioGraph::AllMeterLevels Engine::getMeterLevels() const
{
    if (audioGraph_) return audioGraph_->getMeterLevels();
    return {};
}

void Engine::registerEffectParameters(const juce::String& trackId, int slotIndex, juce::AudioProcessor* effect)
{
    if (!effect) return;

    juce::String prefix = "effects." + trackId + "." + juce::String(slotIndex) + ".";
    juce::String effectName = effect->getName();

    if (effectName == "ParametricEQ") {
        auto* eq = dynamic_cast<ParametricEqProcessor*>(effect);
        if (!eq) return;
        for (int b = 0; b < 4; ++b) {
            juce::String bandPrefix = prefix + "band" + juce::String(b) + ".";
            auto& band = eq->bands[b];
            paramRegistry_.registerParameter(bandPrefix + "frequency", {
                [&band]() -> juce::var { return band.frequency.load(std::memory_order_relaxed); },
                [&band](const juce::var& v) { band.frequency.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
                "float", 20.0, 20000.0
            });
            paramRegistry_.registerParameter(bandPrefix + "q", {
                [&band]() -> juce::var { return band.q.load(std::memory_order_relaxed); },
                [&band](const juce::var& v) { band.q.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
                "float", 0.1, 20.0
            });
            paramRegistry_.registerParameter(bandPrefix + "gainDb", {
                [&band]() -> juce::var { return band.gainDb.load(std::memory_order_relaxed); },
                [&band](const juce::var& v) { band.gainDb.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
                "float", -24.0, 24.0
            });
            paramRegistry_.registerParameter(bandPrefix + "enabled", {
                [&band]() -> juce::var { return band.enabled.load(std::memory_order_relaxed); },
                [&band](const juce::var& v) { band.enabled.store(static_cast<bool>(v), std::memory_order_relaxed); },
                "bool", false, true
            });
        }
    }
    else if (effectName == "Compressor") {
        auto* comp = dynamic_cast<CompressorProcessor*>(effect);
        if (!comp) return;
        paramRegistry_.registerParameter(prefix + "threshold", {
            [comp]() -> juce::var { return comp->threshold.load(std::memory_order_relaxed); },
            [comp](const juce::var& v) { comp->threshold.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
            "float", -60.0, 0.0
        });
        paramRegistry_.registerParameter(prefix + "ratio", {
            [comp]() -> juce::var { return comp->ratio.load(std::memory_order_relaxed); },
            [comp](const juce::var& v) { comp->ratio.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
            "float", 1.0, 20.0
        });
        paramRegistry_.registerParameter(prefix + "attack", {
            [comp]() -> juce::var { return comp->attack.load(std::memory_order_relaxed); },
            [comp](const juce::var& v) { comp->attack.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
            "float", 0.1, 200.0
        });
        paramRegistry_.registerParameter(prefix + "release", {
            [comp]() -> juce::var { return comp->release.load(std::memory_order_relaxed); },
            [comp](const juce::var& v) { comp->release.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
            "float", 10.0, 1000.0
        });
        paramRegistry_.registerParameter(prefix + "makeupGain", {
            [comp]() -> juce::var { return comp->makeupGain.load(std::memory_order_relaxed); },
            [comp](const juce::var& v) { comp->makeupGain.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
            "float", 0.0, 30.0
        });
    }
    else if (effectName == "Reverb") {
        auto* reverb = dynamic_cast<ReverbProcessor*>(effect);
        if (!reverb) return;
        paramRegistry_.registerParameter(prefix + "roomSize", {
            [reverb]() -> juce::var { return reverb->roomSize.load(std::memory_order_relaxed); },
            [reverb](const juce::var& v) { reverb->roomSize.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
            "float", 0.0, 1.0
        });
        paramRegistry_.registerParameter(prefix + "damping", {
            [reverb]() -> juce::var { return reverb->damping.load(std::memory_order_relaxed); },
            [reverb](const juce::var& v) { reverb->damping.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
            "float", 0.0, 1.0
        });
        paramRegistry_.registerParameter(prefix + "wetLevel", {
            [reverb]() -> juce::var { return reverb->wetLevel.load(std::memory_order_relaxed); },
            [reverb](const juce::var& v) { reverb->wetLevel.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
            "float", 0.0, 1.0
        });
        paramRegistry_.registerParameter(prefix + "dryLevel", {
            [reverb]() -> juce::var { return reverb->dryLevel.load(std::memory_order_relaxed); },
            [reverb](const juce::var& v) { reverb->dryLevel.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
            "float", 0.0, 1.0
        });
        paramRegistry_.registerParameter(prefix + "preDelay", {
            [reverb]() -> juce::var { return reverb->preDelay.load(std::memory_order_relaxed); },
            [reverb](const juce::var& v) { reverb->preDelay.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
            "float", 0.0, 250.0
        });
    }
    else if (effectName == "Delay") {
        auto* delay = dynamic_cast<DelayProcessor*>(effect);
        if (!delay) return;
        paramRegistry_.registerParameter(prefix + "feedback", {
            [delay]() -> juce::var { return delay->feedback.load(std::memory_order_relaxed); },
            [delay](const juce::var& v) { delay->feedback.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
            "float", 0.0, 0.95
        });
        paramRegistry_.registerParameter(prefix + "wetDry", {
            [delay]() -> juce::var { return delay->wetDry.load(std::memory_order_relaxed); },
            [delay](const juce::var& v) { delay->wetDry.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
            "float", 0.0, 1.0
        });
        paramRegistry_.registerParameter(prefix + "syncNoteValue", {
            [delay]() -> juce::var { return delay->syncNoteValue.load(std::memory_order_relaxed); },
            [delay](const juce::var& v) { delay->syncNoteValue.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
            "float", 0.125, 4.0
        });
        paramRegistry_.registerParameter(prefix + "pingPongEnabled", {
            [delay]() -> juce::var { return delay->pingPongEnabled.load(std::memory_order_relaxed); },
            [delay](const juce::var& v) { delay->pingPongEnabled.store(static_cast<bool>(v), std::memory_order_relaxed); },
            "bool", false, true
        });
    }
    else if (effectName == "Limiter") {
        auto* lim = dynamic_cast<LimiterProcessor*>(effect);
        if (!lim) return;
        paramRegistry_.registerParameter(prefix + "threshold", {
            [lim]() -> juce::var { return lim->threshold.load(std::memory_order_relaxed); },
            [lim](const juce::var& v) { lim->threshold.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
            "float", -30.0, 0.0
        });
        paramRegistry_.registerParameter(prefix + "release", {
            [lim]() -> juce::var { return lim->release.load(std::memory_order_relaxed); },
            [lim](const juce::var& v) { lim->release.store(static_cast<float>(static_cast<double>(v)), std::memory_order_relaxed); },
            "float", 10.0, 1000.0
        });
    }
}

void Engine::unregisterEffectParameters(const juce::String& trackId, int slotIndex)
{
    juce::String prefix = "effects." + trackId + "." + juce::String(slotIndex) + ".";
    paramRegistry_.removeParametersWithPrefix(prefix);
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
    if (!audioGraph_)
        return TransportStateInfo{ "stopped", 120.0, 4, 4, 0, 0.0, false, 0.0, 0.0 };
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
