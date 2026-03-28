#include "calliope/audio_graph.h"

namespace calliope {

// --- GraphCallback implementation ---

AudioGraph::GraphCallback::GraphCallback(AudioGraph& owner) : owner_(owner) {}

void AudioGraph::GraphCallback::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData, int numInputChannels,
    float* const* outputChannelData, int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context)
{
    // Advance transport if playing
    if (owner_.transport_.getState() == TransportState::Playing) {
        owner_.transport_.advancePosition(numSamples);
    }

    // Delegate to the AudioProcessorPlayer which drives the graph
    owner_.player_.audioDeviceIOCallbackWithContext(
        inputChannelData, numInputChannels,
        outputChannelData, numOutputChannels,
        numSamples, context);
}

void AudioGraph::GraphCallback::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    owner_.player_.audioDeviceAboutToStart(device);
    owner_.transport_.setSampleRate(device->getCurrentSampleRate());
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
    // Ensure message manager exists
    juce::MessageManager::getInstance();

    // Configure and prepare the graph
    graph_.setPlayConfigDetails(0, 2, sampleRate, bufferSize);
    graph_.prepareToPlay(sampleRate, bufferSize);
    graph_.setPlayHead(&transport_);

    // Set transport sample rate
    transport_.setSampleRate(sampleRate);

    // Create output IO processor node
    outputNode_ = graph_.addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));

    // Create MasterBusProcessor
    auto masterProc = std::make_unique<MasterBusProcessor>();
    masterBusPtr_ = masterProc.get();
    masterNode_ = graph_.addNode(std::move(masterProc));

    // Create MetronomeProcessor
    auto metronomeProc = std::make_unique<MetronomeProcessor>();
    metronomePtr_ = metronomeProc.get();
    metronomeNode_ = graph_.addNode(std::move(metronomeProc));

    // Connect metronome -> master (stereo: channels 0 and 1)
    graph_.addConnection({{metronomeNode_->nodeID, 0}, {masterNode_->nodeID, 0}});
    graph_.addConnection({{metronomeNode_->nodeID, 1}, {masterNode_->nodeID, 1}});

    // Create PolySynthProcessor
    auto polySynthProc = std::make_unique<PolySynthProcessor>();
    polySynthPtr_ = polySynthProc.get();
    polySynthNode_ = graph_.addNode(std::move(polySynthProc));

    // Create BassSynthProcessor
    auto bassSynthProc = std::make_unique<BassSynthProcessor>();
    bassSynthPtr_ = bassSynthProc.get();
    bassSynthNode_ = graph_.addNode(std::move(bassSynthProc));

    // Create DrumMachineProcessor
    auto drumMachineProc = std::make_unique<DrumMachineProcessor>();
    drumMachinePtr_ = drumMachineProc.get();
    drumMachineNode_ = graph_.addNode(std::move(drumMachineProc));

    // Connect instruments -> master (stereo: channels 0 and 1)
    graph_.addConnection({{polySynthNode_->nodeID, 0}, {masterNode_->nodeID, 0}});
    graph_.addConnection({{polySynthNode_->nodeID, 1}, {masterNode_->nodeID, 1}});
    graph_.addConnection({{bassSynthNode_->nodeID, 0}, {masterNode_->nodeID, 0}});
    graph_.addConnection({{bassSynthNode_->nodeID, 1}, {masterNode_->nodeID, 1}});
    graph_.addConnection({{drumMachineNode_->nodeID, 0}, {masterNode_->nodeID, 0}});
    graph_.addConnection({{drumMachineNode_->nodeID, 1}, {masterNode_->nodeID, 1}});

    // Connect master -> output (stereo: channels 0 and 1)
    graph_.addConnection({{masterNode_->nodeID, 0}, {outputNode_->nodeID, 0}});
    graph_.addConnection({{masterNode_->nodeID, 1}, {outputNode_->nodeID, 1}});

    // Configure AudioDeviceManager
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    setup.sampleRate = sampleRate;
    setup.bufferSize = bufferSize;
    setup.outputChannels.setRange(0, 2, true);

    auto error = deviceManager_.initialise(0, 2, nullptr, true, {}, &setup);
    if (error.isNotEmpty()) {
        return false;
    }

    // Connect player to graph
    player_.setProcessor(&graph_);

    // Register callback with device manager
    deviceManager_.addAudioCallback(graphCallback_.get());

    // Store config
    currentConfig_.sampleRate = sampleRate;
    currentConfig_.bufferSize = bufferSize;
    initialised_ = true;

    return true;
}

void AudioGraph::shutdown()
{
    deviceManager_.removeAudioCallback(graphCallback_.get());
    player_.setProcessor(nullptr);
    deviceManager_.closeAudioDevice();
    graph_.clear();
    outputNode_ = nullptr;
    masterNode_ = nullptr;
    metronomeNode_ = nullptr;
    polySynthNode_ = nullptr;
    bassSynthNode_ = nullptr;
    drumMachineNode_ = nullptr;
    masterBusPtr_ = nullptr;
    metronomePtr_ = nullptr;
    polySynthPtr_ = nullptr;
    bassSynthPtr_ = nullptr;
    drumMachinePtr_ = nullptr;
    initialised_ = false;
}

bool AudioGraph::isInitialised() const { return initialised_; }

bool AudioGraph::setBufferSize(int newBufferSize)
{
    double sr = currentConfig_.sampleRate;
    shutdown();
    return initialise(sr, newBufferSize);
}

Transport& AudioGraph::getTransport() { return transport_; }
MasterBusProcessor& AudioGraph::getMasterBus() { return *masterBusPtr_; }
MetronomeProcessor& AudioGraph::getMetronome() { return *metronomePtr_; }
AudioConfig AudioGraph::getAudioConfig() const { return currentConfig_; }

PolySynthProcessor& AudioGraph::getPolySynth() { return *polySynthPtr_; }
BassSynthProcessor& AudioGraph::getBassSynth() { return *bassSynthPtr_; }
DrumMachineProcessor& AudioGraph::getDrumMachine() { return *drumMachinePtr_; }

} // namespace calliope
