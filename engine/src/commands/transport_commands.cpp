#include "calliope/commands/transport_commands.h"

namespace calliope {

// --- PlayCommand ---

bool PlayCommand::perform() {
    transport_.play();
    return true;
}

juce::var PlayCommand::getEventData() const {
    auto* obj = new juce::DynamicObject();
    obj->setProperty("action", "play");
    return juce::var(obj);
}

// --- StopCommand ---

bool StopCommand::perform() {
    transport_.stop();
    return true;
}

juce::var StopCommand::getEventData() const {
    auto* obj = new juce::DynamicObject();
    obj->setProperty("action", "stop");
    return juce::var(obj);
}

// --- PauseCommand ---

bool PauseCommand::perform() {
    transport_.pause();
    return true;
}

juce::var PauseCommand::getEventData() const {
    auto* obj = new juce::DynamicObject();
    obj->setProperty("action", "pause");
    return juce::var(obj);
}

// --- SetBpmCommand ---

bool SetBpmCommand::perform() {
    oldBpm_ = transport_.getBpm();
    transport_.setBpm(newBpm_);
    return true;
}

bool SetBpmCommand::undo() {
    transport_.setBpm(oldBpm_);
    return true;
}

juce::var SetBpmCommand::getEventData() const {
    auto* obj = new juce::DynamicObject();
    obj->setProperty("bpm", newBpm_);
    return juce::var(obj);
}

// --- SetTimeSignatureCommand ---

bool SetTimeSignatureCommand::perform() {
    oldNum_ = transport_.getTimeSignatureNumerator();
    oldDen_ = transport_.getTimeSignatureDenominator();
    transport_.setTimeSignature(newNum_, newDen_);
    return true;
}

bool SetTimeSignatureCommand::undo() {
    transport_.setTimeSignature(oldNum_, oldDen_);
    return true;
}

juce::var SetTimeSignatureCommand::getEventData() const {
    auto* obj = new juce::DynamicObject();
    obj->setProperty("numerator", newNum_);
    obj->setProperty("denominator", newDen_);
    return juce::var(obj);
}

// --- SetLoopRegionCommand ---

bool SetLoopRegionCommand::perform() {
    oldStart_ = transport_.getLoopStartBeat();
    oldEnd_ = transport_.getLoopEndBeat();
    oldEnabled_ = transport_.isLooping();
    transport_.setLoopRegion(newStart_, newEnd_, newEnabled_);
    return true;
}

bool SetLoopRegionCommand::undo() {
    transport_.setLoopRegion(oldStart_, oldEnd_, oldEnabled_);
    return true;
}

juce::var SetLoopRegionCommand::getEventData() const {
    auto* obj = new juce::DynamicObject();
    obj->setProperty("startBeat", newStart_);
    obj->setProperty("endBeat", newEnd_);
    obj->setProperty("enabled", newEnabled_);
    return juce::var(obj);
}

// --- SetMetronomeEnabledCommand ---

bool SetMetronomeEnabledCommand::perform() {
    oldEnabled_ = metronome_.enabled.load();
    metronome_.enabled.store(newEnabled_);
    return true;
}

bool SetMetronomeEnabledCommand::undo() {
    metronome_.enabled.store(oldEnabled_);
    return true;
}

juce::var SetMetronomeEnabledCommand::getEventData() const {
    auto* obj = new juce::DynamicObject();
    obj->setProperty("enabled", newEnabled_);
    return juce::var(obj);
}

// --- SetMetronomeVolumeCommand ---

bool SetMetronomeVolumeCommand::perform() {
    oldVolume_ = metronome_.volume.load();
    metronome_.volume.store(newVolume_);
    return true;
}

bool SetMetronomeVolumeCommand::undo() {
    metronome_.volume.store(oldVolume_);
    return true;
}

juce::var SetMetronomeVolumeCommand::getEventData() const {
    auto* obj = new juce::DynamicObject();
    obj->setProperty("volume", static_cast<double>(newVolume_));
    return juce::var(obj);
}

// --- SetMasterVolumeCommand ---

bool SetMasterVolumeCommand::perform() {
    oldVolume_ = masterBus_.masterVolume.load();
    masterBus_.masterVolume.store(newVolume_);
    return true;
}

bool SetMasterVolumeCommand::undo() {
    masterBus_.masterVolume.store(oldVolume_);
    return true;
}

juce::var SetMasterVolumeCommand::getEventData() const {
    auto* obj = new juce::DynamicObject();
    obj->setProperty("volume", static_cast<double>(newVolume_));
    return juce::var(obj);
}

} // namespace calliope
