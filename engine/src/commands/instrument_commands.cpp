#include "calliope/commands/instrument_commands.h"

namespace calliope {

// --- NoteOnCommand ---

bool NoteOnCommand::perform()
{
    // Resolve to the correct instrument type and call addMidiEvent
    if (auto* ps = dynamic_cast<PolySynthProcessor*>(&processor_)) {
        ps->addMidiEvent(juce::MidiMessage::noteOn(1, note_, velocity_));
        return true;
    }
    if (auto* bs = dynamic_cast<BassSynthProcessor*>(&processor_)) {
        bs->addMidiEvent(juce::MidiMessage::noteOn(1, note_, velocity_));
        return true;
    }
    if (auto* dm = dynamic_cast<DrumMachineProcessor*>(&processor_)) {
        dm->addMidiEvent(juce::MidiMessage::noteOn(1, note_, velocity_));
        return true;
    }
    return false;
}

juce::var NoteOnCommand::getEventData() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("note", note_);
    obj->setProperty("velocity", static_cast<double>(velocity_));
    return juce::var(obj);
}

// --- NoteOffCommand ---

bool NoteOffCommand::perform()
{
    if (auto* ps = dynamic_cast<PolySynthProcessor*>(&processor_)) {
        ps->addMidiEvent(juce::MidiMessage::noteOff(1, note_));
        return true;
    }
    if (auto* bs = dynamic_cast<BassSynthProcessor*>(&processor_)) {
        bs->addMidiEvent(juce::MidiMessage::noteOff(1, note_));
        return true;
    }
    if (auto* dm = dynamic_cast<DrumMachineProcessor*>(&processor_)) {
        dm->addMidiEvent(juce::MidiMessage::noteOff(1, note_));
        return true;
    }
    return false;
}

juce::var NoteOffCommand::getEventData() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("note", note_);
    return juce::var(obj);
}

// --- LoadSampleCommand ---

bool LoadSampleCommand::perform()
{
    juce::File file(filePath_);
    return drumMachine_.loadSample(padIndex_, file);
}

juce::var LoadSampleCommand::getEventData() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("padIndex", padIndex_);
    obj->setProperty("filePath", juce::String(filePath_));
    return juce::var(obj);
}

} // namespace calliope
