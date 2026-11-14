#include "calliope/audio_graph.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace calliope {

using UpdateKind = juce::AudioProcessorGraph::UpdateKind;

// --- GraphCallback implementation ---

AudioGraph::GraphCallback::GraphCallback(AudioGraph& owner) : owner_(owner) {}

void AudioGraph::GraphCallback::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData, int numInputChannels,
    float* const* outputChannelData, int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context)
{
    auto& transport = owner_.transport_;
    const bool playing = transport.getState() == TransportState::Playing;

    // Schedule this block's MIDI from the position of its first sample...
    owner_.clipScheduler_.processBlock(
        numSamples,
        transport.getSampleRate(),
        transport.getBpm(),
        transport.getSamplePosition(),
        playing,
        transport.isLooping(),
        transport.getLoopStartBeat(),
        transport.getLoopEndBeat());

    // ...render it...
    owner_.player_.audioDeviceIOCallbackWithContext(
        inputChannelData, numInputChannels,
        outputChannelData, numOutputChannels,
        numSamples, context);

    // ...and only then move the playhead on. Advancing first would skip
    // everything on the very first block, including a note on beat 0.
    if (playing)
        transport.advancePosition(numSamples);
}

void AudioGraph::GraphCallback::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    // The device decides the real rate; it may not be the one we asked for.
    owner_.transport_.setSampleRate(device->getCurrentSampleRate());
    owner_.player_.audioDeviceAboutToStart(device);
}

void AudioGraph::GraphCallback::audioDeviceStopped()
{
    owner_.player_.audioDeviceStopped();
}

// --- AudioGraph implementation ---

AudioGraph::AudioGraph()
    : graphCallback_(std::make_unique<GraphCallback>(*this))
{
}

AudioGraph::~AudioGraph()
{
    if (initialised_)
        shutdown();
}

bool AudioGraph::initialise(double sampleRate, int bufferSize)
{
    std::lock_guard<std::recursive_mutex> lock(graphMutex_);
    if (initialised_)
        return true;

    // Ensure message manager exists
    juce::MessageManager::getInstance();

    graph_.setPlayConfigDetails(0, 2, sampleRate, bufferSize);
    graph_.setPlayHead(&transport_);
    transport_.setSampleRate(sampleRate);

    outputNode_ = graph_.addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode),
        std::nullopt, UpdateKind::none);

    auto masterProc = std::make_unique<MasterBusProcessor>();
    masterBusPtr_ = masterProc.get();
    masterNode_ = graph_.addNode(std::move(masterProc), std::nullopt, UpdateKind::none);

    auto metronomeProc = std::make_unique<MetronomeProcessor>();
    metronomePtr_ = metronomeProc.get();
    metronomeNode_ = graph_.addNode(std::move(metronomeProc), std::nullopt, UpdateKind::none);

    auto masterChainProc = std::make_unique<InsertChainProcessor>("master");
    masterChainPtr_ = masterChainProc.get();
    masterChainNode_ = graph_.addNode(std::move(masterChainProc), std::nullopt, UpdateKind::none);

    bool connected = true;
    for (int ch = 0; ch < 2; ++ch) {
        connected &= graph_.addConnection({{metronomeNode_->nodeID, ch}, {masterNode_->nodeID, ch}}, UpdateKind::none);
        connected &= graph_.addConnection({{masterNode_->nodeID, ch}, {masterChainNode_->nodeID, ch}}, UpdateKind::none);
        connected &= graph_.addConnection({{masterChainNode_->nodeID, ch}, {outputNode_->nodeID, ch}}, UpdateKind::none);
    }
    jassert(connected);
    if (!connected) {
        graph_.clear();
        return false;
    }

    graph_.prepareToPlay(sampleRate, bufferSize);

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    setup.sampleRate = sampleRate;
    setup.bufferSize = bufferSize;
    setup.outputChannels.setRange(0, 2, true);

    auto error = deviceManager_.initialise(0, 2, nullptr, true, {}, &setup);
    if (error.isNotEmpty()) {
        graph_.clear();
        outputNode_ = masterNode_ = metronomeNode_ = masterChainNode_ = nullptr;
        masterBusPtr_ = nullptr;
        metronomePtr_ = nullptr;
        masterChainPtr_ = nullptr;
        return false;
    }

    currentConfig_.sampleRate = sampleRate;
    currentConfig_.bufferSize = bufferSize;
    if (auto* device = deviceManager_.getCurrentAudioDevice()) {
        // Report what the device actually gave us.
        if (device->getCurrentSampleRate() > 0.0)
            currentConfig_.sampleRate = device->getCurrentSampleRate();
        if (device->getCurrentBufferSizeSamples() > 0)
            currentConfig_.bufferSize = device->getCurrentBufferSizeSamples();
    }
    transport_.setSampleRate(currentConfig_.sampleRate);

    player_.setProcessor(&graph_);
    deviceManager_.addAudioCallback(graphCallback_.get());

    initialised_ = true;
    return true;
}

void AudioGraph::shutdown()
{
    std::lock_guard<std::recursive_mutex> lock(graphMutex_);

    deviceManager_.removeAudioCallback(graphCallback_.get());
    player_.setProcessor(nullptr);
    deviceManager_.closeAudioDevice();

    // The scheduler must let go of the instruments before the graph frees them.
    clipScheduler_.clearTrackSinks();
    clipScheduler_.clearClips();
    tracks_.clear();

    graph_.clear();
    outputNode_ = nullptr;
    masterNode_ = nullptr;
    metronomeNode_ = nullptr;
    masterChainNode_ = nullptr;
    masterBusPtr_ = nullptr;
    metronomePtr_ = nullptr;
    masterChainPtr_ = nullptr;
    transport_.stop();
    initialised_ = false;
}

bool AudioGraph::isInitialised() const { return initialised_; }

bool AudioGraph::setBufferSize(int newBufferSize)
{
    std::lock_guard<std::recursive_mutex> lock(graphMutex_);
    if (!initialised_)
        return false;

    auto setup = deviceManager_.getAudioDeviceSetup();
    setup.bufferSize = newBufferSize;
    auto error = deviceManager_.setAudioDeviceSetup(setup, true);
    if (error.isNotEmpty())
        return false;

    currentConfig_.bufferSize = newBufferSize;
    if (auto* device = deviceManager_.getCurrentAudioDevice()) {
        if (device->getCurrentBufferSizeSamples() > 0)
            currentConfig_.bufferSize = device->getCurrentBufferSizeSamples();
        if (device->getCurrentSampleRate() > 0.0)
            currentConfig_.sampleRate = device->getCurrentSampleRate();
    }
    return true;
}

Transport& AudioGraph::getTransport() { return transport_; }
MasterBusProcessor& AudioGraph::getMasterBus() { return *masterBusPtr_; }
MetronomeProcessor& AudioGraph::getMetronome() { return *metronomePtr_; }
AudioConfig AudioGraph::getAudioConfig() const { return currentConfig_; }
ClipScheduler& AudioGraph::getClipScheduler() { return clipScheduler_; }

// Applies pending topology changes (made with UpdateKind::none) right now.
//
// AudioProcessorGraph::rebuild() only rebuilds immediately when called on
// JUCE's message thread; from any other thread it posts the rebuild to the
// JUCE message loop. This engine is hosted inside Node/Electron, where no
// JUCE message loop runs, so a posted rebuild would never happen and the new
// nodes would never be rendered. prepareToPlay() with unchanged settings takes
// the immediate path on any thread: it prepares only the nodes that are not
// prepared yet and swaps in the new render sequence.
void AudioGraph::rebuildGraphNow()
{
    double sampleRate = graph_.getSampleRate();
    int blockSize = graph_.getBlockSize();
    if (sampleRate <= 0.0) sampleRate = currentConfig_.sampleRate;
    if (blockSize <= 0) blockSize = currentConfig_.bufferSize;
    graph_.prepareToPlay(sampleRate, blockSize);
}

// --- Tracks ---

const AudioGraph::Track* AudioGraph::findTrack(const std::string& trackId) const
{
    for (const auto& track : tracks_)
        if (track.id == trackId) return &track;
    return nullptr;
}

AudioGraph::Track* AudioGraph::findTrack(const std::string& trackId)
{
    for (auto& track : tracks_)
        if (track.id == trackId) return &track;
    return nullptr;
}

bool AudioGraph::addTrack(const std::string& trackId, const std::string& instrumentType)
{
    std::lock_guard<std::recursive_mutex> lock(graphMutex_);
    if (!initialised_ || trackId.empty() || trackId == "master" || findTrack(trackId) != nullptr)
        return false;

    Track track;
    track.id = trackId;
    track.instrumentType = instrumentType;

    std::unique_ptr<juce::AudioProcessor> instrument;
    if (instrumentType == InstrumentTypes::polySynth) {
        auto proc = std::make_unique<PolySynthProcessor>();
        track.sink = proc.get();
        instrument = std::move(proc);
    } else if (instrumentType == InstrumentTypes::bassSynth) {
        auto proc = std::make_unique<BassSynthProcessor>();
        track.sink = proc.get();
        instrument = std::move(proc);
    } else if (instrumentType == InstrumentTypes::drumMachine) {
        auto proc = std::make_unique<DrumMachineProcessor>();
        proc->loadDefaultKit();
        track.sink = proc.get();
        instrument = std::move(proc);
    } else {
        return false;
    }
    track.instrument = instrument.get();

    auto channel = std::make_unique<InsertChainProcessor>(juce::String(trackId));
    track.channel = channel.get();

    track.instrumentNode = graph_.addNode(std::move(instrument), std::nullopt, UpdateKind::none);
    track.channelNode = graph_.addNode(std::move(channel), std::nullopt, UpdateKind::none);
    if (track.instrumentNode == nullptr || track.channelNode == nullptr) {
        if (track.instrumentNode) graph_.removeNode(track.instrumentNode.get(), UpdateKind::none);
        if (track.channelNode) graph_.removeNode(track.channelNode.get(), UpdateKind::none);
        rebuildGraphNow();
        return false;
    }

    bool connected = true;
    for (int ch = 0; ch < 2; ++ch) {
        connected &= graph_.addConnection(
            {{track.instrumentNode->nodeID, ch}, {track.channelNode->nodeID, ch}}, UpdateKind::none);
        connected &= graph_.addConnection(
            {{track.channelNode->nodeID, ch}, {masterNode_->nodeID, ch}}, UpdateKind::none);
    }
    jassert(connected);
    if (!connected) {
        graph_.removeNode(track.instrumentNode.get(), UpdateKind::none);
        graph_.removeNode(track.channelNode.get(), UpdateKind::none);
        rebuildGraphNow();
        return false;
    }

    // One rebuild for the whole change; it also prepares the new nodes.
    rebuildGraphNow();

    // The scheduler keeps the instrument node alive for as long as any of its
    // snapshots can still reach it.
    clipScheduler_.setTrackSink(
        trackId, track.sink,
        std::make_shared<juce::AudioProcessorGraph::Node::Ptr>(track.instrumentNode));

    tracks_.push_back(std::move(track));
    applySoloState();
    return true;
}

bool AudioGraph::removeTrack(const std::string& trackId)
{
    std::lock_guard<std::recursive_mutex> lock(graphMutex_);
    auto it = std::find_if(tracks_.begin(), tracks_.end(),
                           [&](const Track& t) { return t.id == trackId; });
    if (it == tracks_.end())
        return false;

    clipScheduler_.removeClipsForTrack(trackId);
    clipScheduler_.removeTrackSink(trackId);

    graph_.removeNode(it->instrumentNode.get(), UpdateKind::none);
    graph_.removeNode(it->channelNode.get(), UpdateKind::none);
    rebuildGraphNow();

    tracks_.erase(it);
    applySoloState();
    return true;
}

void AudioGraph::clearTracks()
{
    std::lock_guard<std::recursive_mutex> lock(graphMutex_);
    clipScheduler_.clearClips();
    clipScheduler_.clearTrackSinks();
    for (auto& track : tracks_) {
        graph_.removeNode(track.instrumentNode.get(), UpdateKind::none);
        graph_.removeNode(track.channelNode.get(), UpdateKind::none);
    }
    if (initialised_)
        rebuildGraphNow();
    tracks_.clear();
}

bool AudioGraph::hasTrack(const std::string& trackId) const
{
    std::lock_guard<std::recursive_mutex> lock(graphMutex_);
    return findTrack(trackId) != nullptr;
}

std::vector<std::string> AudioGraph::getTrackIds() const
{
    std::lock_guard<std::recursive_mutex> lock(graphMutex_);
    std::vector<std::string> ids;
    ids.reserve(tracks_.size());
    for (const auto& track : tracks_)
        ids.push_back(track.id);
    return ids;
}

std::string AudioGraph::getTrackInstrumentType(const std::string& trackId) const
{
    std::lock_guard<std::recursive_mutex> lock(graphMutex_);
    auto* track = findTrack(trackId);
    return track ? track->instrumentType : std::string();
}

juce::AudioProcessor* AudioGraph::getTrackInstrument(const std::string& trackId) const
{
    std::lock_guard<std::recursive_mutex> lock(graphMutex_);
    auto* track = findTrack(trackId);
    return track ? track->instrument : nullptr;
}

MidiSink* AudioGraph::getTrackMidiSink(const std::string& trackId) const
{
    std::lock_guard<std::recursive_mutex> lock(graphMutex_);
    auto* track = findTrack(trackId);
    return track ? track->sink : nullptr;
}

InsertChainProcessor* AudioGraph::getTrackChannel(const std::string& trackId) const
{
    std::lock_guard<std::recursive_mutex> lock(graphMutex_);
    auto* track = findTrack(trackId);
    return track ? track->channel : nullptr;
}

void AudioGraph::setTrackSolo(const std::string& trackId, bool solo)
{
    std::lock_guard<std::recursive_mutex> lock(graphMutex_);
    if (auto* track = findTrack(trackId)) {
        track->solo = solo;
        applySoloState();
    }
}

bool AudioGraph::isTrackSoloed(const std::string& trackId) const
{
    std::lock_guard<std::recursive_mutex> lock(graphMutex_);
    auto* track = findTrack(trackId);
    return track != nullptr && track->solo;
}

void AudioGraph::applySoloState()
{
    const bool anySolo = std::any_of(tracks_.begin(), tracks_.end(),
                                     [](const Track& t) { return t.solo; });
    for (auto& track : tracks_)
        track.channel->soloMuted.store(anySolo && !track.solo, std::memory_order_relaxed);
}

InsertChainProcessor* AudioGraph::findInsertChainProcessor(const juce::String& trackId) const
{
    if (trackId == "master")
        return masterChainPtr_;
    return getTrackChannel(trackId.toStdString());
}

InsertChain* AudioGraph::findInsertChain(const juce::String& trackId) const
{
    auto* processor = findInsertChainProcessor(trackId);
    return processor ? &processor->getInsertChain() : nullptr;
}

// --- Metering ---

AudioGraph::AllMeterLevels AudioGraph::getMeterLevels() const
{
    AllMeterLevels result;
    result.master.trackId = "master";

    std::unique_lock<std::recursive_mutex> lock(graphMutex_, std::try_to_lock);
    if (!lock.owns_lock() || !initialised_)
        return result;

    for (const auto& track : tracks_) {
        const auto& data = track.channel->getMeterData();
        AllMeterLevels::TrackMeter m;
        m.trackId = track.id;
        m.rmsLeft = data.rmsLeft.load(std::memory_order_relaxed);
        m.rmsRight = data.rmsRight.load(std::memory_order_relaxed);
        m.peakLeft = data.peakLeft.load(std::memory_order_relaxed);
        m.peakRight = data.peakRight.load(std::memory_order_relaxed);
        result.tracks.push_back(std::move(m));
    }

    // Post-insert, so the meter shows what actually reaches the output.
    const auto& master = masterChainPtr_->getMeterData();
    result.master.rmsLeft = master.rmsLeft.load(std::memory_order_relaxed);
    result.master.rmsRight = master.rmsRight.load(std::memory_order_relaxed);
    result.master.peakLeft = master.peakLeft.load(std::memory_order_relaxed);
    result.master.peakRight = master.peakRight.load(std::memory_order_relaxed);
    return result;
}

// --- Offline rendering ---

juce::AudioBuffer<float> AudioGraph::renderOffline(double totalBeats, double tailSeconds,
                                                   const std::string& soloTrackId,
                                                   const std::function<void(float)>& progress)
{
    std::lock_guard<std::recursive_mutex> lock(graphMutex_);
    if (!initialised_ || totalBeats <= 0.0)
        return {};

    const double sampleRate = currentConfig_.sampleRate;
    const double bpm = transport_.getBpm();
    if (sampleRate <= 0.0 || bpm <= 0.0)
        return {};

    const auto musicSamples = static_cast<int64_t>(std::ceil(totalBeats * 60.0 / bpm * sampleRate));
    const auto totalSamples = musicSamples
        + static_cast<int64_t>(std::max(0.0, tailSeconds) * sampleRate);
    if (totalSamples <= 0 || totalSamples > std::numeric_limits<int>::max())
        return {};

    // Take the graph away from the device. removeAudioCallback returns only
    // once the device thread is out of our callback.
    deviceManager_.removeAudioCallback(graphCallback_.get());

    // Remember everything we are about to disturb.
    const bool wasLooping = transport_.isLooping();
    const double loopStart = transport_.getLoopStartBeat();
    const double loopEnd = transport_.getLoopEndBeat();
    const bool metronomeWasEnabled = metronomePtr_->enabled.load();
    std::vector<bool> previousSoloMuted;
    previousSoloMuted.reserve(tracks_.size());
    for (auto& track : tracks_)
        previousSoloMuted.push_back(track.channel->soloMuted.load());

    transport_.stop();
    transport_.setLoopRegion(loopStart, loopEnd, false);
    metronomePtr_->enabled.store(false);
    if (!soloTrackId.empty()) {
        for (auto& track : tracks_)
            track.channel->soloMuted.store(track.id != soloTrackId);
    }

    const int blockSize = 512;
    juce::AudioBuffer<float> output(2, static_cast<int>(totalSamples));
    output.clear();
    juce::AudioBuffer<float> block(2, blockSize);
    juce::MidiBuffer midi;

    // Detaching the device released the graph's resources, so it has to be
    // prepared again before it can render. (The graph is deliberately left in
    // real-time mode: in non-real-time mode JUCE's graph blocks inside
    // processBlock until it is prepared, which would hang here if this
    // prepare were ever skipped.)
    graph_.setPlayConfigDetails(0, 2, sampleRate, blockSize);
    graph_.prepareToPlay(sampleRate, blockSize);
    graph_.reset();

    // Let the scheduler see a stopped transport so it starts from a clean slate,
    // then flush the resulting note-offs through the instruments.
    clipScheduler_.processBlock(blockSize, sampleRate, bpm, 0, false, false, 0.0, 0.0);
    block.clear();
    graph_.processBlock(block, midi);

    transport_.play();
    int64_t rendered = 0;
    while (rendered < totalSamples) {
        const int numSamples = static_cast<int>(
            std::min<int64_t>(blockSize, totalSamples - rendered));
        // Past the end of the music only tails ring out.
        const bool playing = rendered < musicSamples;

        clipScheduler_.processBlock(numSamples, sampleRate, bpm,
                                    transport_.getSamplePosition(),
                                    playing, false, 0.0, 0.0);

        block.setSize(2, numSamples, false, false, true);
        block.clear();
        midi.clear();
        graph_.processBlock(block, midi);

        for (int ch = 0; ch < 2; ++ch)
            output.copyFrom(ch, static_cast<int>(rendered), block, ch, 0, numSamples);

        transport_.advancePosition(numSamples);
        rendered += numSamples;

        if (progress)
            progress(static_cast<float>(rendered) / static_cast<float>(totalSamples));
    }

    // Put everything back and hand the graph to the device again.
    transport_.stop();
    clipScheduler_.processBlock(blockSize, sampleRate, bpm, 0, false, false, 0.0, 0.0);
    graph_.reset();

    transport_.setLoopRegion(loopStart, loopEnd, wasLooping);
    metronomePtr_->enabled.store(metronomeWasEnabled);
    for (size_t i = 0; i < tracks_.size(); ++i)
        tracks_[i].channel->soloMuted.store(previousSoloMuted[i]);

    deviceManager_.addAudioCallback(graphCallback_.get());
    return output;
}

} // namespace calliope
