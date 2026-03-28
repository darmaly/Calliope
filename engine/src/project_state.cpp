#include "calliope/project_state.h"
#include "calliope/engine.h"

namespace calliope {

juce::String ProjectState::toJson() const
{
    auto* root = new juce::DynamicObject();

    // Transport
    auto* transportObj = new juce::DynamicObject();
    transportObj->setProperty("state", transport.state);
    transportObj->setProperty("bpm", transport.bpm);
    transportObj->setProperty("timeSigNumerator", transport.timeSigNumerator);
    transportObj->setProperty("timeSigDenominator", transport.timeSigDenominator);
    transportObj->setProperty("looping", transport.looping);
    transportObj->setProperty("loopStartBeat", transport.loopStartBeat);
    transportObj->setProperty("loopEndBeat", transport.loopEndBeat);
    root->setProperty("transport", juce::var(transportObj));

    // Metronome
    auto* metronomeObj = new juce::DynamicObject();
    metronomeObj->setProperty("enabled", metronome.enabled);
    metronomeObj->setProperty("volume", static_cast<double>(metronome.volume));
    root->setProperty("metronome", juce::var(metronomeObj));

    // MasterBus
    auto* masterBusObj = new juce::DynamicObject();
    masterBusObj->setProperty("volume", static_cast<double>(masterBus.volume));
    root->setProperty("masterBus", juce::var(masterBusObj));

    // AudioConfig
    auto* audioConfigObj = new juce::DynamicObject();
    audioConfigObj->setProperty("sampleRate", audioConfig.sampleRate);
    audioConfigObj->setProperty("bufferSize", audioConfig.bufferSize);
    root->setProperty("audioConfig", juce::var(audioConfigObj));

    // Instruments
    auto* instrumentsObj = new juce::DynamicObject();

    // PolySynth
    auto* psObj = new juce::DynamicObject();
    psObj->setProperty("osc1Waveform", polySynth.osc1Waveform);
    psObj->setProperty("osc2Waveform", polySynth.osc2Waveform);
    psObj->setProperty("oscMix", static_cast<double>(polySynth.oscMix));
    psObj->setProperty("osc2Detune", static_cast<double>(polySynth.osc2Detune));
    psObj->setProperty("filterCutoff", static_cast<double>(polySynth.filterCutoff));
    psObj->setProperty("filterResonance", static_cast<double>(polySynth.filterResonance));
    psObj->setProperty("filterEnvAmount", static_cast<double>(polySynth.filterEnvAmount));
    psObj->setProperty("ampAttack", static_cast<double>(polySynth.ampAttack));
    psObj->setProperty("ampDecay", static_cast<double>(polySynth.ampDecay));
    psObj->setProperty("ampSustain", static_cast<double>(polySynth.ampSustain));
    psObj->setProperty("ampRelease", static_cast<double>(polySynth.ampRelease));
    psObj->setProperty("filterAttack", static_cast<double>(polySynth.filterAttack));
    psObj->setProperty("filterDecay", static_cast<double>(polySynth.filterDecay));
    psObj->setProperty("filterSustain", static_cast<double>(polySynth.filterSustain));
    psObj->setProperty("filterRelease", static_cast<double>(polySynth.filterRelease));
    psObj->setProperty("lfoRate", static_cast<double>(polySynth.lfoRate));
    psObj->setProperty("lfoDepth", static_cast<double>(polySynth.lfoDepth));
    psObj->setProperty("lfoTarget", polySynth.lfoTarget);
    psObj->setProperty("masterGain", static_cast<double>(polySynth.masterGain));
    instrumentsObj->setProperty("polysynth", juce::var(psObj));

    // BassSynth
    auto* bsObj = new juce::DynamicObject();
    bsObj->setProperty("oscWaveform", bassSynth.oscWaveform);
    bsObj->setProperty("subOscMix", static_cast<double>(bassSynth.subOscMix));
    bsObj->setProperty("subOscOctave", bassSynth.subOscOctave);
    bsObj->setProperty("filterCutoff", static_cast<double>(bassSynth.filterCutoff));
    bsObj->setProperty("filterResonance", static_cast<double>(bassSynth.filterResonance));
    bsObj->setProperty("filterEnvAmount", static_cast<double>(bassSynth.filterEnvAmount));
    bsObj->setProperty("ampAttack", static_cast<double>(bassSynth.ampAttack));
    bsObj->setProperty("ampDecay", static_cast<double>(bassSynth.ampDecay));
    bsObj->setProperty("ampSustain", static_cast<double>(bassSynth.ampSustain));
    bsObj->setProperty("ampRelease", static_cast<double>(bassSynth.ampRelease));
    bsObj->setProperty("filterAttack", static_cast<double>(bassSynth.filterAttack));
    bsObj->setProperty("filterDecay", static_cast<double>(bassSynth.filterDecay));
    bsObj->setProperty("filterSustain", static_cast<double>(bassSynth.filterSustain));
    bsObj->setProperty("filterRelease", static_cast<double>(bassSynth.filterRelease));
    bsObj->setProperty("masterGain", static_cast<double>(bassSynth.masterGain));
    instrumentsObj->setProperty("basssynth", juce::var(bsObj));

    // DrumMachine
    auto* dmObj = new juce::DynamicObject();
    dmObj->setProperty("volume", static_cast<double>(drumMachine.volume));
    juce::Array<juce::var> pads;
    for (const auto& name : drumMachine.padNames)
        pads.add(juce::var(name));
    dmObj->setProperty("pads", juce::var(pads));
    instrumentsObj->setProperty("drumMachine", juce::var(dmObj));

    root->setProperty("instruments", juce::var(instrumentsObj));

    return juce::JSON::toString(juce::var(root));
}

bool ProjectState::fromJson(const juce::String& jsonString)
{
    auto parsed = juce::JSON::parse(jsonString);
    if (!parsed.isObject())
        return false;

    auto* root = parsed.getDynamicObject();
    if (root == nullptr)
        return false;

    // Transport
    auto transportVar = root->getProperty("transport");
    if (auto* transportObj = transportVar.getDynamicObject())
    {
        transport.state = transportObj->getProperty("state").toString();
        transport.bpm = static_cast<double>(transportObj->getProperty("bpm"));
        transport.timeSigNumerator = static_cast<int>(transportObj->getProperty("timeSigNumerator"));
        transport.timeSigDenominator = static_cast<int>(transportObj->getProperty("timeSigDenominator"));
        transport.looping = static_cast<bool>(transportObj->getProperty("looping"));
        transport.loopStartBeat = static_cast<double>(transportObj->getProperty("loopStartBeat"));
        transport.loopEndBeat = static_cast<double>(transportObj->getProperty("loopEndBeat"));
    }

    // Metronome
    auto metronomeVar = root->getProperty("metronome");
    if (auto* metronomeObj = metronomeVar.getDynamicObject())
    {
        metronome.enabled = static_cast<bool>(metronomeObj->getProperty("enabled"));
        metronome.volume = static_cast<float>(static_cast<double>(metronomeObj->getProperty("volume")));
    }

    // MasterBus
    auto masterBusVar = root->getProperty("masterBus");
    if (auto* masterBusObj = masterBusVar.getDynamicObject())
    {
        masterBus.volume = static_cast<float>(static_cast<double>(masterBusObj->getProperty("volume")));
    }

    // AudioConfig
    auto audioConfigVar = root->getProperty("audioConfig");
    if (auto* audioConfigObj = audioConfigVar.getDynamicObject())
    {
        audioConfig.sampleRate = static_cast<double>(audioConfigObj->getProperty("sampleRate"));
        audioConfig.bufferSize = static_cast<int>(audioConfigObj->getProperty("bufferSize"));
    }

    // Instruments
    auto instrumentsVar = root->getProperty("instruments");
    if (auto* instrumentsObj = instrumentsVar.getDynamicObject())
    {
        // PolySynth
        auto psVar = instrumentsObj->getProperty("polysynth");
        if (auto* psObj = psVar.getDynamicObject())
        {
            polySynth.osc1Waveform = static_cast<int>(psObj->getProperty("osc1Waveform"));
            polySynth.osc2Waveform = static_cast<int>(psObj->getProperty("osc2Waveform"));
            polySynth.oscMix = static_cast<float>(static_cast<double>(psObj->getProperty("oscMix")));
            polySynth.osc2Detune = static_cast<float>(static_cast<double>(psObj->getProperty("osc2Detune")));
            polySynth.filterCutoff = static_cast<float>(static_cast<double>(psObj->getProperty("filterCutoff")));
            polySynth.filterResonance = static_cast<float>(static_cast<double>(psObj->getProperty("filterResonance")));
            polySynth.filterEnvAmount = static_cast<float>(static_cast<double>(psObj->getProperty("filterEnvAmount")));
            polySynth.ampAttack = static_cast<float>(static_cast<double>(psObj->getProperty("ampAttack")));
            polySynth.ampDecay = static_cast<float>(static_cast<double>(psObj->getProperty("ampDecay")));
            polySynth.ampSustain = static_cast<float>(static_cast<double>(psObj->getProperty("ampSustain")));
            polySynth.ampRelease = static_cast<float>(static_cast<double>(psObj->getProperty("ampRelease")));
            polySynth.filterAttack = static_cast<float>(static_cast<double>(psObj->getProperty("filterAttack")));
            polySynth.filterDecay = static_cast<float>(static_cast<double>(psObj->getProperty("filterDecay")));
            polySynth.filterSustain = static_cast<float>(static_cast<double>(psObj->getProperty("filterSustain")));
            polySynth.filterRelease = static_cast<float>(static_cast<double>(psObj->getProperty("filterRelease")));
            polySynth.lfoRate = static_cast<float>(static_cast<double>(psObj->getProperty("lfoRate")));
            polySynth.lfoDepth = static_cast<float>(static_cast<double>(psObj->getProperty("lfoDepth")));
            polySynth.lfoTarget = static_cast<int>(psObj->getProperty("lfoTarget"));
            polySynth.masterGain = static_cast<float>(static_cast<double>(psObj->getProperty("masterGain")));
        }

        // BassSynth
        auto bsVar = instrumentsObj->getProperty("basssynth");
        if (auto* bsObj = bsVar.getDynamicObject())
        {
            bassSynth.oscWaveform = static_cast<int>(bsObj->getProperty("oscWaveform"));
            bassSynth.subOscMix = static_cast<float>(static_cast<double>(bsObj->getProperty("subOscMix")));
            bassSynth.subOscOctave = static_cast<int>(bsObj->getProperty("subOscOctave"));
            bassSynth.filterCutoff = static_cast<float>(static_cast<double>(bsObj->getProperty("filterCutoff")));
            bassSynth.filterResonance = static_cast<float>(static_cast<double>(bsObj->getProperty("filterResonance")));
            bassSynth.filterEnvAmount = static_cast<float>(static_cast<double>(bsObj->getProperty("filterEnvAmount")));
            bassSynth.ampAttack = static_cast<float>(static_cast<double>(bsObj->getProperty("ampAttack")));
            bassSynth.ampDecay = static_cast<float>(static_cast<double>(bsObj->getProperty("ampDecay")));
            bassSynth.ampSustain = static_cast<float>(static_cast<double>(bsObj->getProperty("ampSustain")));
            bassSynth.ampRelease = static_cast<float>(static_cast<double>(bsObj->getProperty("ampRelease")));
            bassSynth.filterAttack = static_cast<float>(static_cast<double>(bsObj->getProperty("filterAttack")));
            bassSynth.filterDecay = static_cast<float>(static_cast<double>(bsObj->getProperty("filterDecay")));
            bassSynth.filterSustain = static_cast<float>(static_cast<double>(bsObj->getProperty("filterSustain")));
            bassSynth.filterRelease = static_cast<float>(static_cast<double>(bsObj->getProperty("filterRelease")));
            bassSynth.masterGain = static_cast<float>(static_cast<double>(bsObj->getProperty("masterGain")));
        }

        // DrumMachine
        auto dmVar = instrumentsObj->getProperty("drumMachine");
        if (auto* dmObj = dmVar.getDynamicObject())
        {
            drumMachine.volume = static_cast<float>(static_cast<double>(dmObj->getProperty("volume")));
            drumMachine.padNames.clear();
            auto padsVar = dmObj->getProperty("pads");
            if (auto* padsArray = padsVar.getArray()) {
                for (const auto& p : *padsArray)
                    drumMachine.padNames.push_back(p.toString());
            }
        }
    }

    return true;
}

void ProjectState::snapshotFromEngine(const Engine& engine)
{
    auto transportInfo = engine.getTransportState();
    transport.state = juce::String(transportInfo.state);
    transport.bpm = transportInfo.bpm;
    transport.timeSigNumerator = transportInfo.timeSigNumerator;
    transport.timeSigDenominator = transportInfo.timeSigDenominator;
    transport.looping = transportInfo.looping;
    transport.loopStartBeat = transportInfo.loopStartBeat;
    transport.loopEndBeat = transportInfo.loopEndBeat;

    auto audioInfo = engine.getAudioConfig();
    audioConfig.sampleRate = audioInfo.sampleRate;
    audioConfig.bufferSize = audioInfo.bufferSize;

    // Access metronome and master bus via audio graph
    // Note: const_cast needed because Engine::getAudioGraph() is non-const
    auto& mutableEngine = const_cast<Engine&>(engine);
    auto& graph = mutableEngine.getAudioGraph();
    metronome.enabled = graph.getMetronome().enabled.load();
    metronome.volume = graph.getMetronome().volume.load();
    masterBus.volume = graph.getMasterBus().masterVolume.load();

    // Instruments
    auto& ps = graph.getPolySynth();
    polySynth.osc1Waveform = ps.osc1Waveform.load();
    polySynth.osc2Waveform = ps.osc2Waveform.load();
    polySynth.oscMix = ps.oscMix.load();
    polySynth.osc2Detune = ps.osc2Detune.load();
    polySynth.filterCutoff = ps.filterCutoff.load();
    polySynth.filterResonance = ps.filterResonance.load();
    polySynth.filterEnvAmount = ps.filterEnvAmount.load();
    polySynth.ampAttack = ps.ampAttack.load();
    polySynth.ampDecay = ps.ampDecay.load();
    polySynth.ampSustain = ps.ampSustain.load();
    polySynth.ampRelease = ps.ampRelease.load();
    polySynth.filterAttack = ps.filterAttack.load();
    polySynth.filterDecay = ps.filterDecay.load();
    polySynth.filterSustain = ps.filterSustain.load();
    polySynth.filterRelease = ps.filterRelease.load();
    polySynth.lfoRate = ps.lfoRate.load();
    polySynth.lfoDepth = ps.lfoDepth.load();
    polySynth.lfoTarget = ps.lfoTarget.load();
    polySynth.masterGain = ps.masterGain.load();

    auto& bsSynth = graph.getBassSynth();
    bassSynth.oscWaveform = bsSynth.oscWaveform.load();
    bassSynth.subOscMix = bsSynth.subOscMix.load();
    bassSynth.subOscOctave = bsSynth.subOscOctave.load();
    bassSynth.filterCutoff = bsSynth.filterCutoff.load();
    bassSynth.filterResonance = bsSynth.filterResonance.load();
    bassSynth.filterEnvAmount = bsSynth.filterEnvAmount.load();
    bassSynth.ampAttack = bsSynth.ampAttack.load();
    bassSynth.ampDecay = bsSynth.ampDecay.load();
    bassSynth.ampSustain = bsSynth.ampSustain.load();
    bassSynth.ampRelease = bsSynth.ampRelease.load();
    bassSynth.filterAttack = bsSynth.filterAttack.load();
    bassSynth.filterDecay = bsSynth.filterDecay.load();
    bassSynth.filterSustain = bsSynth.filterSustain.load();
    bassSynth.filterRelease = bsSynth.filterRelease.load();
    bassSynth.masterGain = bsSynth.masterGain.load();

    auto& dm = graph.getDrumMachine();
    drumMachine.volume = dm.volume.load();
    drumMachine.padNames.clear();
    auto names = dm.getSampleNames();
    for (const auto& n : names)
        drumMachine.padNames.push_back(n);
}

} // namespace calliope
