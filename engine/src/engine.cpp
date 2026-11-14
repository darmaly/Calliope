#include "calliope/engine.h"
#include "calliope/project_state.h"
#include "calliope/audio_exporter.h"
#include "calliope/commands/effect_commands.h"
#include "calliope/effects/parametric_eq.h"
#include "calliope/effects/compressor.h"
#include "calliope/effects/reverb.h"
#include "calliope/effects/delay.h"
#include "calliope/effects/limiter.h"
#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <cassert>

namespace calliope {

// ---------------------------------------------------------------------------
// Parameter registration helpers
//
// Every setter clamps to the declared range: parameters are driven by the UI
// and by an AI agent, and an out-of-range cutoff or gain must not be able to
// destabilise a filter or blow up the output.
// ---------------------------------------------------------------------------

namespace {

void registerFloat(ParameterRegistry& registry, const juce::String& id,
                   std::atomic<float>& value, double minValue, double maxValue)
{
    registry.registerParameter(id, {
        [&value]() -> juce::var { return static_cast<double>(value.load(std::memory_order_relaxed)); },
        [&value, minValue, maxValue](const juce::var& v) {
            const double clamped = juce::jlimit(minValue, maxValue, static_cast<double>(v));
            value.store(static_cast<float>(clamped), std::memory_order_relaxed);
        },
        "float", minValue, maxValue
    });
}

void registerInt(ParameterRegistry& registry, const juce::String& id,
                 std::atomic<int>& value, int minValue, int maxValue)
{
    registry.registerParameter(id, {
        [&value]() -> juce::var { return value.load(std::memory_order_relaxed); },
        [&value, minValue, maxValue](const juce::var& v) {
            const int rounded = juce::roundToInt(static_cast<double>(v));
            value.store(juce::jlimit(minValue, maxValue, rounded), std::memory_order_relaxed);
        },
        "int", minValue, maxValue
    });
}

void registerBool(ParameterRegistry& registry, const juce::String& id, std::atomic<bool>& value)
{
    registry.registerParameter(id, {
        [&value]() -> juce::var { return value.load(std::memory_order_relaxed); },
        [&value](const juce::var& v) { value.store(static_cast<bool>(v), std::memory_order_relaxed); },
        "bool", false, true
    });
}

void registerPolySynth(ParameterRegistry& r, const juce::String& p, PolySynthProcessor& s)
{
    registerInt(r, p + "osc1Waveform", s.osc1Waveform, 0, 3);
    registerInt(r, p + "osc2Waveform", s.osc2Waveform, 0, 3);
    registerFloat(r, p + "oscMix", s.oscMix, 0.0, 1.0);
    registerFloat(r, p + "osc2Detune", s.osc2Detune, -100.0, 100.0);
    registerFloat(r, p + "filterCutoff", s.filterCutoff, 20.0, 20000.0);
    registerFloat(r, p + "filterResonance", s.filterResonance, 0.0, 1.0);
    registerFloat(r, p + "filterEnvAmount", s.filterEnvAmount, 0.0, 1.0);
    registerFloat(r, p + "ampAttack", s.ampAttack, 0.001, 10.0);
    registerFloat(r, p + "ampDecay", s.ampDecay, 0.001, 10.0);
    registerFloat(r, p + "ampSustain", s.ampSustain, 0.0, 1.0);
    registerFloat(r, p + "ampRelease", s.ampRelease, 0.001, 10.0);
    registerFloat(r, p + "filterAttack", s.filterAttack, 0.001, 10.0);
    registerFloat(r, p + "filterDecay", s.filterDecay, 0.001, 10.0);
    registerFloat(r, p + "filterSustain", s.filterSustain, 0.0, 1.0);
    registerFloat(r, p + "filterRelease", s.filterRelease, 0.001, 10.0);
    registerFloat(r, p + "lfoRate", s.lfoRate, 0.1, 20.0);
    registerFloat(r, p + "lfoDepth", s.lfoDepth, 0.0, 1.0);
    registerInt(r, p + "lfoTarget", s.lfoTarget, 0, 2);
    registerFloat(r, p + "masterGain", s.masterGain, 0.0, 1.0);
}

void registerBassSynth(ParameterRegistry& r, const juce::String& p, BassSynthProcessor& s)
{
    registerInt(r, p + "oscWaveform", s.oscWaveform, 0, 3);
    registerFloat(r, p + "subOscMix", s.subOscMix, 0.0, 1.0);
    registerInt(r, p + "subOscOctave", s.subOscOctave, -2, -1);
    registerFloat(r, p + "filterCutoff", s.filterCutoff, 20.0, 5000.0);
    registerFloat(r, p + "filterResonance", s.filterResonance, 0.0, 1.0);
    registerFloat(r, p + "filterEnvAmount", s.filterEnvAmount, 0.0, 1.0);
    registerFloat(r, p + "ampAttack", s.ampAttack, 0.001, 10.0);
    registerFloat(r, p + "ampDecay", s.ampDecay, 0.001, 10.0);
    registerFloat(r, p + "ampSustain", s.ampSustain, 0.0, 1.0);
    registerFloat(r, p + "ampRelease", s.ampRelease, 0.001, 10.0);
    registerFloat(r, p + "filterAttack", s.filterAttack, 0.001, 10.0);
    registerFloat(r, p + "filterDecay", s.filterDecay, 0.001, 10.0);
    registerFloat(r, p + "filterSustain", s.filterSustain, 0.0, 1.0);
    registerFloat(r, p + "filterRelease", s.filterRelease, 0.001, 10.0);
    registerFloat(r, p + "masterGain", s.masterGain, 0.0, 1.0);
}

} // namespace

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
    // Guard against concurrent initialisation from multiple bridge threads
    std::lock_guard<std::mutex> lock(initMutex_);
    if (initialised_.load()) return true;

    auto graph = std::make_unique<AudioGraph>();
    if (!graph->initialise(sampleRate, bufferSize))
        return false;

    audioGraph_ = std::move(graph);
    audioExporter_ = std::make_unique<AudioExporter>(*this);
    registerGlobalParameters();
    initialised_.store(true);
    return true;
}

void Engine::shutdown()
{
    std::lock_guard<std::mutex> lock(initMutex_);
    initialised_.store(false);

    // Undo actions and parameter accessors point into the graph.
    dispatcher_.clearHistory();
    paramRegistry_.clear();

    audioExporter_.reset();
    if (audioGraph_) {
        audioGraph_->shutdown();
        audioGraph_.reset();
    }
}

bool Engine::isInitialised() const
{
    return initialised_.load();
}

// --- Sub-component access ---

AudioGraph& Engine::getAudioGraph()
{
    assert(audioGraph_ && "Engine not initialised");
    return *audioGraph_;
}

Transport& Engine::getTransport()
{
    return getAudioGraph().getTransport();
}

ClipScheduler& Engine::getClipScheduler()
{
    return getAudioGraph().getClipScheduler();
}

InsertChain* Engine::findInsertChain(const juce::String& trackId)
{
    if (!audioGraph_) return nullptr;
    return audioGraph_->findInsertChain(trackId);
}

// --- Tracks ---

bool Engine::addTrack(const std::string& trackId, const std::string& instrumentType)
{
    if (!isInitialised()) return false;
    if (!audioGraph_->addTrack(trackId, instrumentType))
        return false;
    registerTrackParameters(trackId);
    return true;
}

bool Engine::removeTrack(const std::string& trackId)
{
    if (!isInitialised() || !audioGraph_->hasTrack(trackId))
        return false;

    // Undo actions may hold references into this track's chain.
    dispatcher_.clearHistory();
    unregisterTrackParameters(trackId);
    return audioGraph_->removeTrack(trackId);
}

void Engine::resetProject()
{
    if (!isInitialised()) return;

    dispatcher_.clearHistory();

    auto& graph = *audioGraph_;
    graph.getTransport().stop();

    for (const auto& trackId : graph.getTrackIds())
        unregisterTrackParameters(trackId);
    graph.clearTracks();

    if (auto* masterChain = graph.findInsertChain("master")) {
        paramRegistry_.removeParametersWithPrefix("effects.master.");
        while (masterChain->getNumEffects() > 0)
            masterChain->removeEffect(masterChain->getNumEffects() - 1);
    }

    graph.getTransport().setBpm(120.0);
    graph.getTransport().setTimeSignature(4, 4);
    graph.getTransport().setLoopRegion(0.0, 0.0, false);
    graph.getMetronome().enabled.store(true);
    graph.getMetronome().volume.store(0.7f);
    graph.getMasterBus().masterVolume.store(1.0f);
}

void Engine::registerTrackParameters(const std::string& trackId)
{
    auto& graph = *audioGraph_;
    const juce::String prefix = "track." + juce::String(trackId) + ".";

    if (auto* channel = graph.getTrackChannel(trackId)) {
        registerFloat(paramRegistry_, prefix + "volume", channel->volume, 0.0, 2.0);
        registerFloat(paramRegistry_, prefix + "pan", channel->pan, -1.0, 1.0);
        registerBool(paramRegistry_, prefix + "mute", channel->mute);
    }

    // Solo is a property of the whole mix, not of one channel.
    paramRegistry_.registerParameter(prefix + "solo", {
        [this, trackId]() -> juce::var { return audioGraph_ && audioGraph_->isTrackSoloed(trackId); },
        [this, trackId](const juce::var& v) { if (audioGraph_) audioGraph_->setTrackSolo(trackId, static_cast<bool>(v)); },
        "bool", false, true
    });

    auto* instrument = graph.getTrackInstrument(trackId);
    if (auto* polySynth = dynamic_cast<PolySynthProcessor*>(instrument)) {
        registerPolySynth(paramRegistry_, prefix, *polySynth);
    } else if (auto* bassSynth = dynamic_cast<BassSynthProcessor*>(instrument)) {
        registerBassSynth(paramRegistry_, prefix, *bassSynth);
    } else if (auto* drumMachine = dynamic_cast<DrumMachineProcessor*>(instrument)) {
        registerFloat(paramRegistry_, prefix + "kitVolume", drumMachine->volume, 0.0, 1.0);
    }
}

void Engine::unregisterTrackParameters(const std::string& trackId)
{
    paramRegistry_.removeParametersWithPrefix("track." + juce::String(trackId) + ".");
    paramRegistry_.removeParametersWithPrefix("effects." + juce::String(trackId) + ".");
}

void Engine::registerEffectParameters(const juce::String& trackId, int slotIndex, juce::AudioProcessor* effect)
{
    if (!effect) return;

    const juce::String prefix = "effects." + trackId + "." + juce::String(slotIndex) + ".";
    auto& r = paramRegistry_;

    if (auto* eq = dynamic_cast<ParametricEqProcessor*>(effect)) {
        for (int b = 0; b < 4; ++b) {
            const juce::String bandPrefix = prefix + "band" + juce::String(b) + ".";
            auto& band = eq->bands[b];
            registerFloat(r, bandPrefix + "frequency", band.frequency, 20.0, 20000.0);
            registerFloat(r, bandPrefix + "q", band.q, 0.1, 20.0);
            registerFloat(r, bandPrefix + "gainDb", band.gainDb, -24.0, 24.0);
            registerBool(r, bandPrefix + "enabled", band.enabled);
        }
    }
    else if (auto* comp = dynamic_cast<CompressorProcessor*>(effect)) {
        registerFloat(r, prefix + "threshold", comp->threshold, -60.0, 0.0);
        registerFloat(r, prefix + "ratio", comp->ratio, 1.0, 20.0);
        registerFloat(r, prefix + "attack", comp->attack, 0.1, 200.0);
        registerFloat(r, prefix + "release", comp->release, 10.0, 1000.0);
        registerFloat(r, prefix + "makeupGain", comp->makeupGain, 0.0, 30.0);
    }
    else if (auto* reverb = dynamic_cast<ReverbProcessor*>(effect)) {
        registerFloat(r, prefix + "roomSize", reverb->roomSize, 0.0, 1.0);
        registerFloat(r, prefix + "damping", reverb->damping, 0.0, 1.0);
        registerFloat(r, prefix + "wetLevel", reverb->wetLevel, 0.0, 1.0);
        registerFloat(r, prefix + "dryLevel", reverb->dryLevel, 0.0, 1.0);
        registerFloat(r, prefix + "preDelay", reverb->preDelay, 0.0, 250.0);
    }
    else if (auto* delay = dynamic_cast<DelayProcessor*>(effect)) {
        registerFloat(r, prefix + "feedback", delay->feedback, 0.0, 0.95);
        registerFloat(r, prefix + "wetDry", delay->wetDry, 0.0, 1.0);
        registerFloat(r, prefix + "syncNoteValue", delay->syncNoteValue, 0.125, 4.0);
        registerBool(r, prefix + "pingPongEnabled", delay->pingPongEnabled);
    }
    else if (auto* lim = dynamic_cast<LimiterProcessor*>(effect)) {
        registerFloat(r, prefix + "threshold", lim->threshold, -30.0, 0.0);
        registerFloat(r, prefix + "release", lim->release, 10.0, 1000.0);
    }
}

void Engine::unregisterEffectParameters(const juce::String& trackId, int slotIndex)
{
    juce::String prefix = "effects." + trackId + "." + juce::String(slotIndex) + ".";
    paramRegistry_.removeParametersWithPrefix(prefix);
}

AudioGraph::AllMeterLevels Engine::getMeterLevels() const
{
    if (!isInitialised()) return {};
    return audioGraph_->getMeterLevels();
}

AudioExporter& Engine::getAudioExporter()
{
    assert(audioExporter_ && "Engine not initialised");
    return *audioExporter_;
}

// --- Convenience wrappers ---

void Engine::transportPlay()    { getTransport().play(); }
void Engine::transportStop()    { getTransport().stop(); }
void Engine::transportPause()   { getTransport().pause(); }
void Engine::setBpm(double bpm) { getTransport().setBpm(juce::jlimit(20.0, 999.0, bpm)); }

void Engine::setTimeSignature(int numerator, int denominator)
{
    getTransport().setTimeSignature(juce::jlimit(1, 32, numerator), juce::jlimit(1, 32, denominator));
}

void Engine::setLoopRegion(double startBeat, double endBeat, bool enabled)
{
    // A loop needs a positive length; an empty one would pin the playhead.
    getTransport().setLoopRegion(startBeat, endBeat, enabled && endBeat > startBeat);
}

bool Engine::setBufferSize(int bufferSize)
{
    if (!isInitialised()) return false;
    return audioGraph_->setBufferSize(bufferSize);
}

void Engine::setMetronomeEnabled(bool enabled)
{
    getAudioGraph().getMetronome().enabled.store(enabled, std::memory_order_relaxed);
}

void Engine::setMetronomeVolume(float volume)
{
    getAudioGraph().getMetronome().volume.store(juce::jlimit(0.0f, 1.0f, volume), std::memory_order_relaxed);
}

// --- State queries ---

Engine::TransportStateInfo Engine::getTransportState() const
{
    if (!isInitialised())
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
    if (isInitialised()) {
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

bool Engine::applyProjectState(const ProjectState& state)
{
    if (!isInitialised()) return false;

    resetProject();
    auto& graph = *audioGraph_;
    bool ok = true;

    for (const auto& track : state.tracks)
        ok &= addTrack(track.id.toStdString(), track.instrument.toStdString());

    for (const auto& chainData : state.effectChains) {
        auto* chain = graph.findInsertChain(chainData.trackId);
        if (chain == nullptr) { ok = false; continue; }

        for (const auto& slot : chainData.effects) {
            auto effect = createEffect(slot.effectType);
            if (!effect) { ok = false; continue; }
            const int index = chain->insertEffect(std::move(effect));
            registerEffectParameters(chainData.trackId, index, chain->getEffect(index));
            if (slot.bypassed)
                chain->setBypass(index, true);
        }
    }

    for (const auto& clip : state.clips)
        graph.getClipScheduler().addClip(clip);

    setBpm(state.transport.bpm);
    setTimeSignature(state.transport.timeSigNumerator, state.transport.timeSigDenominator);
    setLoopRegion(state.transport.loopStartBeat, state.transport.loopEndBeat, state.transport.looping);
    setMetronomeEnabled(state.metronome.enabled);
    setMetronomeVolume(state.metronome.volume);
    graph.getMasterBus().masterVolume.store(juce::jlimit(0.0f, 2.0f, state.masterBus.volume));

    // Every remaining value (instrument, mixer and effect settings) is a
    // registered parameter. Unknown IDs are skipped so older or newer files load.
    if (auto* params = state.parameters.getDynamicObject()) {
        for (const auto& prop : params->getProperties()) {
            if (const auto* def = paramRegistry_.getParameter(prop.name.toString()))
                def->setter(prop.value);
        }
    }

    return ok;
}

void Engine::registerGlobalParameters()
{
    paramRegistry_.registerParameter("transport.bpm", {
        [this]() -> juce::var { return getTransport().getBpm(); },
        [this](const juce::var& v) { setBpm(static_cast<double>(v)); },
        "double", 20.0, 999.0
    });

    paramRegistry_.registerParameter("transport.timeSigNumerator", {
        [this]() -> juce::var { return getTransport().getTimeSignatureNumerator(); },
        [this](const juce::var& v) { setTimeSignature(static_cast<int>(v), getTransport().getTimeSignatureDenominator()); },
        "int", 1, 32
    });

    paramRegistry_.registerParameter("transport.timeSigDenominator", {
        [this]() -> juce::var { return getTransport().getTimeSignatureDenominator(); },
        [this](const juce::var& v) { setTimeSignature(getTransport().getTimeSignatureNumerator(), static_cast<int>(v)); },
        "int", 1, 32
    });

    auto& graph = *audioGraph_;
    registerBool(paramRegistry_, "metronome.enabled", graph.getMetronome().enabled);
    registerFloat(paramRegistry_, "metronome.volume", graph.getMetronome().volume, 0.0, 1.0);
    registerFloat(paramRegistry_, "master.volume", graph.getMasterBus().masterVolume, 0.0, 2.0);
}

// --- Static methods (preserved from Phase 1) ---

std::string Engine::getJuceVersion()
{
    return juce::SystemStats::getJUCEVersion().toStdString();
}

std::vector<std::string> Engine::getAudioDevices()
{
    // Guard against concurrent access with initialise() — both touch JUCE audio system
    std::lock_guard<std::mutex> lock(getInstance().initMutex_);
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
