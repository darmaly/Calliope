import { useTimelineStore } from '../stores/timeline-store'

/**
 * Create a new clip on a track and dispatch to engine.
 */
export function createClip(
  trackId: string,
  startBeat: number,
  lengthBeats: number,
  type: 'midi' | 'audio',
): void {
  const name = type === 'midi' ? 'MIDI Clip' : 'Audio Clip'

  // Capture clip IDs before add to identify the new one
  const clipsBefore = Object.keys(useTimelineStore.getState().clips)

  useTimelineStore.getState().addClip({
    trackId,
    type,
    startBeat,
    lengthBeats,
    name,
  })

  // Dispatch MIDI clip to C++ engine for audio playback
  if (type === 'midi') {
    const clipsAfter = useTimelineStore.getState().clips
    const newClipId = Object.keys(clipsAfter).find((id) => !clipsBefore.includes(id))
    if (newClipId) {
      window.calliope?.clipAdd({
        clipId: newClipId,
        trackId,
        startBeat,
        lengthBeats,
        notes: [],
      }).catch(() => {})
    }
  }
}

/**
 * Delete a clip by ID.
 */
export function deleteClip(clipId: string): void {
  useTimelineStore.getState().removeClip(clipId)

  // Remove clip from C++ engine
  window.calliope?.clipRemove(clipId).catch(() => {})
}

/**
 * Move a clip to a new beat position, optionally to a different track.
 */
export function moveClip(clipId: string, startBeat: number, trackId?: string): void {
  useTimelineStore.getState().moveClip(clipId, startBeat, trackId)

  // Sync updated clip to C++ engine
  const clip = useTimelineStore.getState().clips[clipId]
  if (clip && clip.type === 'midi') {
    const notes = clip.notes
      ? Object.values(clip.notes).map((n) => ({
          pitch: n.pitch,
          startBeat: n.startBeat,
          lengthBeats: n.lengthBeats,
          velocity: n.velocity,
        }))
      : []
    window.calliope?.clipUpdate({
      clipId,
      trackId: clip.trackId,
      startBeat: clip.startBeat,
      lengthBeats: clip.lengthBeats,
      notes,
    }).catch(() => {})
  }
}

/**
 * Resize a clip (change start beat and/or length).
 */
export function resizeClip(clipId: string, startBeat: number, lengthBeats: number): void {
  useTimelineStore.getState().resizeClip(clipId, startBeat, lengthBeats)

  // Sync updated clip to C++ engine
  const clip = useTimelineStore.getState().clips[clipId]
  if (clip && clip.type === 'midi') {
    const notes = clip.notes
      ? Object.values(clip.notes).map((n) => ({
          pitch: n.pitch,
          startBeat: n.startBeat,
          lengthBeats: n.lengthBeats,
          velocity: n.velocity,
        }))
      : []
    window.calliope?.clipUpdate({
      clipId,
      trackId: clip.trackId,
      startBeat: clip.startBeat,
      lengthBeats: clip.lengthBeats,
      notes,
    }).catch(() => {})
  }
}

/**
 * Duplicate a clip (placed immediately after the original).
 */
export function duplicateClip(clipId: string): void {
  // Capture clip IDs before duplicate to identify the new one
  const clipsBefore = Object.keys(useTimelineStore.getState().clips)
  const sourceClip = useTimelineStore.getState().clips[clipId]

  useTimelineStore.getState().duplicateClip(clipId)

  // Sync duplicated MIDI clip to C++ engine
  if (sourceClip && sourceClip.type === 'midi') {
    const clipsAfter = useTimelineStore.getState().clips
    const newClipId = Object.keys(clipsAfter).find((id) => !clipsBefore.includes(id))
    if (newClipId) {
      const newClip = clipsAfter[newClipId]
      const notes = newClip.notes
        ? Object.values(newClip.notes).map((n) => ({
            pitch: n.pitch,
            startBeat: n.startBeat,
            lengthBeats: n.lengthBeats,
            velocity: n.velocity,
          }))
        : []
      window.calliope?.clipAdd({
        clipId: newClipId,
        trackId: newClip.trackId,
        startBeat: newClip.startBeat,
        lengthBeats: newClip.lengthBeats,
        notes,
      }).catch(() => {})
    }
  }
}
