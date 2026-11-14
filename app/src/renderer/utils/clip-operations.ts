import { useTimelineStore } from '../stores/timeline-store'

// Clip edits change the project document. project/engine-sync notices and
// updates the audio engine; nothing here talks to the engine directly.

/** Create a new clip on a track. Returns the new clip's ID. */
export function createClip(
  trackId: string,
  startBeat: number,
  lengthBeats: number,
  type: 'midi' | 'audio',
): string {
  return useTimelineStore.getState().addClip({
    trackId,
    type,
    startBeat,
    lengthBeats,
    name: type === 'midi' ? 'MIDI Clip' : 'Audio Clip',
    ...(type === 'midi' ? { notes: {} } : {}),
  })
}

/** Delete a clip by ID. */
export function deleteClip(clipId: string): void {
  useTimelineStore.getState().removeClip(clipId)
}

/** Move a clip to a new beat position, optionally to a different track. */
export function moveClip(clipId: string, startBeat: number, trackId?: string): void {
  useTimelineStore.getState().moveClip(clipId, startBeat, trackId)
}

/** Resize a clip (change start beat and/or length). */
export function resizeClip(clipId: string, startBeat: number, lengthBeats: number): void {
  useTimelineStore.getState().resizeClip(clipId, startBeat, lengthBeats)
}

/** Duplicate a clip (placed immediately after the original). Returns the copy's ID. */
export function duplicateClip(clipId: string): string | null {
  return useTimelineStore.getState().duplicateClip(clipId)
}
