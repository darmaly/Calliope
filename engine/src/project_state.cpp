#include "calliope/project_state.h"
#include "calliope/engine.h"
#include "calliope/commands/effect_commands.h"

namespace calliope {

namespace {

juce::var getOr(const juce::DynamicObject& obj, const char* name, const juce::var& fallback)
{
    return obj.hasProperty(name) ? obj.getProperty(name) : fallback;
}

} // namespace

juce::String ProjectState::toJson() const
{
    auto* root = new juce::DynamicObject();
    root->setProperty("formatVersion", formatVersion);

    auto* transportObj = new juce::DynamicObject();
    transportObj->setProperty("state", transport.state);
    transportObj->setProperty("bpm", transport.bpm);
    transportObj->setProperty("timeSigNumerator", transport.timeSigNumerator);
    transportObj->setProperty("timeSigDenominator", transport.timeSigDenominator);
    transportObj->setProperty("looping", transport.looping);
    transportObj->setProperty("loopStartBeat", transport.loopStartBeat);
    transportObj->setProperty("loopEndBeat", transport.loopEndBeat);
    root->setProperty("transport", juce::var(transportObj));

    auto* metronomeObj = new juce::DynamicObject();
    metronomeObj->setProperty("enabled", metronome.enabled);
    metronomeObj->setProperty("volume", static_cast<double>(metronome.volume));
    root->setProperty("metronome", juce::var(metronomeObj));

    auto* masterBusObj = new juce::DynamicObject();
    masterBusObj->setProperty("volume", static_cast<double>(masterBus.volume));
    root->setProperty("masterBus", juce::var(masterBusObj));

    auto* audioConfigObj = new juce::DynamicObject();
    audioConfigObj->setProperty("sampleRate", audioConfig.sampleRate);
    audioConfigObj->setProperty("bufferSize", audioConfig.bufferSize);
    root->setProperty("audioConfig", juce::var(audioConfigObj));

    juce::Array<juce::var> tracksArray;
    for (const auto& track : tracks) {
        auto* trackObj = new juce::DynamicObject();
        trackObj->setProperty("id", track.id);
        trackObj->setProperty("instrument", track.instrument);
        tracksArray.add(juce::var(trackObj));
    }
    root->setProperty("tracks", juce::var(tracksArray));

    juce::Array<juce::var> chainsArray;
    for (const auto& chainData : effectChains) {
        auto* chainObj = new juce::DynamicObject();
        chainObj->setProperty("trackId", chainData.trackId);

        juce::Array<juce::var> effectsArray;
        for (const auto& slot : chainData.effects) {
            auto* slotObj = new juce::DynamicObject();
            slotObj->setProperty("effectType", slot.effectType);
            slotObj->setProperty("bypassed", slot.bypassed);
            effectsArray.add(juce::var(slotObj));
        }
        chainObj->setProperty("effects", juce::var(effectsArray));
        chainsArray.add(juce::var(chainObj));
    }
    root->setProperty("effectChains", juce::var(chainsArray));

    juce::Array<juce::var> clipsArray;
    for (const auto& clip : clips) {
        auto* clipObj = new juce::DynamicObject();
        clipObj->setProperty("clipId", juce::String(clip.clipId));
        clipObj->setProperty("trackId", juce::String(clip.trackId));
        clipObj->setProperty("startBeat", clip.startBeat);
        clipObj->setProperty("lengthBeats", clip.lengthBeats);

        juce::Array<juce::var> notesArray;
        for (const auto& note : clip.notes) {
            auto* noteObj = new juce::DynamicObject();
            noteObj->setProperty("pitch", note.pitch);
            noteObj->setProperty("startBeat", note.startBeat);
            noteObj->setProperty("lengthBeats", note.lengthBeats);
            noteObj->setProperty("velocity", static_cast<double>(note.velocity));
            notesArray.add(juce::var(noteObj));
        }
        clipObj->setProperty("notes", juce::var(notesArray));
        clipsArray.add(juce::var(clipObj));
    }
    root->setProperty("clips", juce::var(clipsArray));

    root->setProperty("parameters",
                      parameters.isObject() ? parameters : juce::var(new juce::DynamicObject()));

    return juce::JSON::toString(juce::var(root));
}

bool ProjectState::fromJson(const juce::String& jsonString)
{
    auto parsed = juce::JSON::parse(jsonString);
    auto* root = parsed.getDynamicObject();
    if (root == nullptr)
        return false;

    *this = ProjectState();
    formatVersion = static_cast<int>(getOr(*root, "formatVersion", 1));

    if (auto* obj = root->getProperty("transport").getDynamicObject()) {
        transport.state = getOr(*obj, "state", transport.state).toString();
        transport.bpm = static_cast<double>(getOr(*obj, "bpm", transport.bpm));
        transport.timeSigNumerator = static_cast<int>(getOr(*obj, "timeSigNumerator", transport.timeSigNumerator));
        transport.timeSigDenominator = static_cast<int>(getOr(*obj, "timeSigDenominator", transport.timeSigDenominator));
        transport.looping = static_cast<bool>(getOr(*obj, "looping", transport.looping));
        transport.loopStartBeat = static_cast<double>(getOr(*obj, "loopStartBeat", transport.loopStartBeat));
        transport.loopEndBeat = static_cast<double>(getOr(*obj, "loopEndBeat", transport.loopEndBeat));
    }

    if (auto* obj = root->getProperty("metronome").getDynamicObject()) {
        metronome.enabled = static_cast<bool>(getOr(*obj, "enabled", metronome.enabled));
        metronome.volume = static_cast<float>(static_cast<double>(getOr(*obj, "volume", static_cast<double>(metronome.volume))));
    }

    if (auto* obj = root->getProperty("masterBus").getDynamicObject())
        masterBus.volume = static_cast<float>(static_cast<double>(getOr(*obj, "volume", static_cast<double>(masterBus.volume))));

    if (auto* obj = root->getProperty("audioConfig").getDynamicObject()) {
        audioConfig.sampleRate = static_cast<double>(getOr(*obj, "sampleRate", audioConfig.sampleRate));
        audioConfig.bufferSize = static_cast<int>(getOr(*obj, "bufferSize", audioConfig.bufferSize));
    }

    if (auto* tracksArray = root->getProperty("tracks").getArray()) {
        for (const auto& trackVar : *tracksArray) {
            if (auto* obj = trackVar.getDynamicObject()) {
                TrackData track;
                track.id = obj->getProperty("id").toString();
                track.instrument = obj->getProperty("instrument").toString();
                if (track.id.isNotEmpty())
                    tracks.push_back(std::move(track));
            }
        }
    }

    if (auto* chainsArray = root->getProperty("effectChains").getArray()) {
        for (const auto& chainVar : *chainsArray) {
            auto* chainObj = chainVar.getDynamicObject();
            if (chainObj == nullptr) continue;

            InsertChainData chainData;
            chainData.trackId = chainObj->getProperty("trackId").toString();
            if (auto* effectsArray = chainObj->getProperty("effects").getArray()) {
                for (const auto& slotVar : *effectsArray) {
                    if (auto* slotObj = slotVar.getDynamicObject()) {
                        EffectSlotData slot;
                        slot.effectType = slotObj->getProperty("effectType").toString();
                        slot.bypassed = static_cast<bool>(slotObj->getProperty("bypassed"));
                        chainData.effects.push_back(std::move(slot));
                    }
                }
            }
            effectChains.push_back(std::move(chainData));
        }
    }

    if (auto* clipsArray = root->getProperty("clips").getArray()) {
        for (const auto& clipVar : *clipsArray) {
            auto* clipObj = clipVar.getDynamicObject();
            if (clipObj == nullptr) continue;

            MidiClipData clip;
            clip.clipId = clipObj->getProperty("clipId").toString().toStdString();
            clip.trackId = clipObj->getProperty("trackId").toString().toStdString();
            clip.startBeat = static_cast<double>(clipObj->getProperty("startBeat"));
            clip.lengthBeats = static_cast<double>(clipObj->getProperty("lengthBeats"));
            if (auto* notesArray = clipObj->getProperty("notes").getArray()) {
                for (const auto& noteVar : *notesArray) {
                    if (auto* noteObj = noteVar.getDynamicObject()) {
                        MidiNoteData note;
                        note.pitch = static_cast<int>(noteObj->getProperty("pitch"));
                        note.startBeat = static_cast<double>(noteObj->getProperty("startBeat"));
                        note.lengthBeats = static_cast<double>(noteObj->getProperty("lengthBeats"));
                        note.velocity = static_cast<float>(static_cast<double>(noteObj->getProperty("velocity")));
                        clip.notes.push_back(note);
                    }
                }
            }
            if (!clip.clipId.empty())
                clips.push_back(std::move(clip));
        }
    }

    auto parametersVar = root->getProperty("parameters");
    if (parametersVar.isObject())
        parameters = parametersVar;

    return true;
}

void ProjectState::snapshotFromEngine(const Engine& engine)
{
    *this = ProjectState();

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

    if (!engine.isInitialised())
        return;

    // The accessors below are non-const on Engine but do not modify it.
    auto& mutableEngine = const_cast<Engine&>(engine);
    auto& graph = mutableEngine.getAudioGraph();

    metronome.enabled = graph.getMetronome().enabled.load();
    metronome.volume = graph.getMetronome().volume.load();
    masterBus.volume = graph.getMasterBus().masterVolume.load();

    std::vector<juce::String> chainIds;
    for (const auto& trackId : graph.getTrackIds()) {
        TrackData track;
        track.id = juce::String(trackId);
        track.instrument = juce::String(graph.getTrackInstrumentType(trackId));
        chainIds.push_back(track.id);
        tracks.push_back(std::move(track));
    }
    chainIds.push_back("master");

    for (const auto& chainId : chainIds) {
        auto* chain = graph.findInsertChain(chainId);
        if (chain == nullptr || chain->getNumEffects() == 0) continue;

        InsertChainData chainData;
        chainData.trackId = chainId;
        for (int i = 0; i < chain->getNumEffects(); ++i) {
            auto* effect = chain->getEffect(i);
            if (effect == nullptr) continue;
            EffectSlotData slot;
            slot.effectType = effectTypeForProcessor(*effect);
            slot.bypassed = chain->isBypassed(i);
            chainData.effects.push_back(std::move(slot));
        }
        effectChains.push_back(std::move(chainData));
    }

    clips = graph.getClipScheduler().getClips();

    auto* parametersObj = new juce::DynamicObject();
    auto& registry = mutableEngine.getParameterRegistry();
    for (const auto& id : registry.getAllParameterIds()) {
        if (const auto* def = registry.getParameter(id))
            parametersObj->setProperty(id, def->getter());
    }
    parameters = juce::var(parametersObj);
}

} // namespace calliope
