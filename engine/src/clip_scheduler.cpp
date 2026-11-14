#include "calliope/clip_scheduler.h"
#include <algorithm>
#include <cmath>

namespace calliope {

namespace {

int64_t beatToTick(double beat)
{
    return static_cast<int64_t>(std::llround(beat * 1000000.0));
}

int64_t tickToSample(int64_t tick, double samplesPerBeat)
{
    return static_cast<int64_t>(
        std::llround(static_cast<double>(tick) * samplesPerBeat / 1000000.0));
}

} // namespace

ClipScheduler::ClipScheduler()
{
    std::lock_guard<std::mutex> lock(editMutex_);
    publish();
}

ClipScheduler::~ClipScheduler() = default;

// ---------------------------------------------------------------------------
// Control thread
// ---------------------------------------------------------------------------

void ClipScheduler::addClip(const MidiClipData& clip)
{
    std::lock_guard<std::mutex> lock(editMutex_);
    clips_[clip.clipId] = clip;
    publish();
}

void ClipScheduler::updateClip(const MidiClipData& clip)
{
    addClip(clip);
}

void ClipScheduler::removeClip(const std::string& clipId)
{
    std::lock_guard<std::mutex> lock(editMutex_);
    if (clips_.erase(clipId) > 0)
        publish();
}

void ClipScheduler::clearClips()
{
    std::lock_guard<std::mutex> lock(editMutex_);
    clips_.clear();
    publish();
}

void ClipScheduler::removeClipsForTrack(const std::string& trackId)
{
    std::lock_guard<std::mutex> lock(editMutex_);
    bool changed = false;
    for (auto it = clips_.begin(); it != clips_.end();) {
        if (it->second.trackId == trackId) {
            it = clips_.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }
    if (changed)
        publish();
}

int ClipScheduler::getClipCount() const
{
    std::lock_guard<std::mutex> lock(editMutex_);
    return static_cast<int>(clips_.size());
}

std::vector<MidiClipData> ClipScheduler::getClips() const
{
    std::lock_guard<std::mutex> lock(editMutex_);
    std::vector<MidiClipData> result;
    result.reserve(clips_.size());
    for (const auto& [id, clip] : clips_)
        result.push_back(clip);
    return result;
}

double ClipScheduler::getEndBeat() const
{
    std::lock_guard<std::mutex> lock(editMutex_);
    double end = 0.0;
    for (const auto& [id, clip] : clips_)
        end = std::max(end, clip.startBeat + clip.lengthBeats);
    return end;
}

void ClipScheduler::setTrackSink(const std::string& trackId, MidiSink* sink,
                                 std::shared_ptr<void> keepAlive)
{
    std::lock_guard<std::mutex> lock(editMutex_);
    sinks_[trackId] = SinkEntry{sink, std::move(keepAlive)};
    publish();
}

void ClipScheduler::removeTrackSink(const std::string& trackId)
{
    std::lock_guard<std::mutex> lock(editMutex_);
    if (sinks_.erase(trackId) > 0)
        publish();
}

void ClipScheduler::clearTrackSinks()
{
    std::lock_guard<std::mutex> lock(editMutex_);
    sinks_.clear();
    publish();
}

void ClipScheduler::publish()
{
    auto snapshot = std::make_shared<Snapshot>();

    std::map<std::string, int> sinkIndexByTrack;
    for (const auto& [trackId, entry] : sinks_) {
        if (entry.sink == nullptr) continue;
        sinkIndexByTrack[trackId] = static_cast<int>(snapshot->sinks.size());
        snapshot->sinks.push_back(entry.sink);
        snapshot->keepAlive.push_back(entry.keepAlive);
    }

    for (const auto& [clipId, clip] : clips_) {
        auto sinkIt = sinkIndexByTrack.find(clip.trackId);
        if (sinkIt == sinkIndexByTrack.end()) continue;  // track has no instrument yet

        const double clipEnd = clip.startBeat + clip.lengthBeats;
        for (const auto& note : clip.notes) {
            if (note.startBeat < 0.0 || note.lengthBeats <= 0.0) continue;
            if (note.pitch < 0 || note.pitch > 127) continue;

            const double onBeat = clip.startBeat + note.startBeat;
            if (onBeat < 0.0 || onBeat >= clipEnd) continue;  // outside the clip
            // A note that overhangs its clip is cut at the clip's end.
            const double offBeat = std::min(onBeat + note.lengthBeats, clipEnd);

            const int64_t onTick = beatToTick(onBeat);
            const int64_t offTick = std::max(beatToTick(offBeat), onTick + 1);
            const float velocity = juce::jlimit(1.0f / 127.0f, 1.0f, note.velocity);

            snapshot->events.push_back({onTick, sinkIt->second, note.pitch, velocity, true});
            snapshot->events.push_back({offTick, sinkIt->second, note.pitch, 0.0f, false});
        }
    }

    // Offs sort before ons on the same tick so a repeated note is released
    // before it is struck again.
    std::stable_sort(snapshot->events.begin(), snapshot->events.end(),
                     [](const Event& a, const Event& b) {
                         if (a.tick != b.tick) return a.tick < b.tick;
                         return !a.noteOn && b.noteOn;
                     });

    std::shared_ptr<const Snapshot> next = std::move(snapshot);
    auto previous = std::atomic_load(&current_);
    std::atomic_store(&current_, next);

    // Keep superseded snapshots until the audio thread has let go of them, so
    // their memory (and the instruments they keep alive) is released here and
    // not on the audio thread.
    if (previous)
        retired_.push_back(std::move(previous));
    retired_.erase(std::remove_if(retired_.begin(), retired_.end(),
                                  [](const std::shared_ptr<const Snapshot>& s) {
                                      return s.use_count() == 1;
                                  }),
                   retired_.end());
}

// ---------------------------------------------------------------------------
// Audio thread
// ---------------------------------------------------------------------------

void ClipScheduler::allNotesOff(const Snapshot& snapshot, int sampleOffset)
{
    const auto message = juce::MidiMessage::allNotesOff(1);
    for (auto* sink : snapshot.sinks)
        sink->addMidiEvent(message, sampleOffset);
}

void ClipScheduler::emitRange(const Snapshot& snapshot, double samplesPerBeat,
                              int64_t rangeStart, int64_t rangeEnd, int bufferOffset)
{
    const auto& events = snapshot.events;
    auto it = std::lower_bound(events.begin(), events.end(), rangeStart,
                               [samplesPerBeat](const Event& e, int64_t sample) {
                                   return tickToSample(e.tick, samplesPerBeat) < sample;
                               });

    for (; it != events.end(); ++it) {
        const int64_t sample = tickToSample(it->tick, samplesPerBeat);
        if (sample >= rangeEnd) break;

        const int offset = bufferOffset + static_cast<int>(sample - rangeStart);
        auto* sink = snapshot.sinks[static_cast<size_t>(it->sinkIndex)];
        if (it->noteOn)
            sink->addMidiEvent(juce::MidiMessage::noteOn(1, it->pitch, it->velocity), offset);
        else
            sink->addMidiEvent(juce::MidiMessage::noteOff(1, it->pitch), offset);
    }
}

void ClipScheduler::processBlock(int numSamples,
                                 double sampleRate,
                                 double bpm,
                                 int64_t blockStartSample,
                                 bool isPlaying,
                                 bool isLooping,
                                 double loopStartBeat,
                                 double loopEndBeat)
{
    auto latest = std::atomic_load(&current_);

    if (!isPlaying) {
        if (wasPlaying_ && active_)
            allNotesOff(*active_, 0);
        wasPlaying_ = false;
        expectedNextSample_ = -1;
        active_ = std::move(latest);
        return;
    }

    if (numSamples <= 0 || sampleRate <= 0.0 || bpm <= 0.0)
        return;

    // The arrangement changed under a playing transport: release whatever the
    // old arrangement had sounding, since its note-offs may no longer exist.
    if (latest != active_) {
        if (wasPlaying_ && active_)
            allNotesOff(*active_, 0);
        active_ = std::move(latest);
    }
    if (!active_)
        return;

    // The playhead jumped (seek, or a loop boundary handled elsewhere).
    if (wasPlaying_ && expectedNextSample_ >= 0 && blockStartSample != expectedNextSample_)
        allNotesOff(*active_, 0);
    wasPlaying_ = true;

    const double samplesPerBeat = sampleRate * 60.0 / bpm;

    // Same arithmetic as Transport::advancePosition, so both agree on the
    // exact sample at which the loop wraps.
    const double beatsPerSecond = bpm / 60.0;
    const auto loopEnd = static_cast<int64_t>((loopEndBeat / beatsPerSecond) * sampleRate);
    const auto loopStart = static_cast<int64_t>((loopStartBeat / beatsPerSecond) * sampleRate);
    const bool loopValid = isLooping && loopEnd > loopStart;

    int64_t position = blockStartSample;
    int offset = 0;
    int remaining = numSamples;

    while (remaining > 0) {
        int64_t segmentEnd = position + remaining;
        const bool wraps = loopValid && position < loopEnd && segmentEnd >= loopEnd;
        if (wraps)
            segmentEnd = loopEnd;

        emitRange(*active_, samplesPerBeat, position, segmentEnd, offset);

        const int rendered = static_cast<int>(segmentEnd - position);
        offset += rendered;
        remaining -= rendered;

        if (wraps) {
            allNotesOff(*active_, std::min(offset, numSamples - 1));
            position = loopStart;
        } else {
            position = segmentEnd;
        }
    }

    expectedNextSample_ = position;
}

} // namespace calliope
