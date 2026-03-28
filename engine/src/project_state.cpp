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
}

} // namespace calliope
