#include "calliope/commands/instrument_commands.h"

namespace calliope {

// --- NoteOnCommand ---

bool NoteOnCommand::perform()
{
    // Every built-in instrument is a MidiSink.
    if (auto* sink = dynamic_cast<MidiSink*>(&processor_)) {
        sink->addMidiEvent(juce::MidiMessage::noteOn(1, juce::jlimit(0, 127, note_),
                                                     juce::jlimit(0.0f, 1.0f, velocity_)));
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
    if (auto* sink = dynamic_cast<MidiSink*>(&processor_)) {
        sink->addMidiEvent(juce::MidiMessage::noteOff(1, juce::jlimit(0, 127, note_)));
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
