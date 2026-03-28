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

  useTimelineStore.getState().addClip({
    trackId,
    type,
    startBeat,
    lengthBeats,
    name,
  })

  // Attempt engine dispatch (engine may not have clip commands yet)
  try {
    window.calliope?.dispatchCommand({
      command: 'clip:create',
      params: { trackId, startBeat, lengthBeats, type, name },
    })
  } catch {
    // Engine clip commands not yet implemented — store-only for now
  }
}

/**
 * Delete a clip by ID.
 */
export function deleteClip(clipId: string): void {
  useTimelineStore.getState().removeClip(clipId)

  try {
    window.calliope?.dispatchCommand({
      command: 'clip:delete',
      params: { clipId },
    })
  } catch {
    // Engine clip commands not yet implemented
  }
}

/**
 * Move a clip to a new beat position, optionally to a different track.
 */
export function moveClip(clipId: string, startBeat: number, trackId?: string): void {
  useTimelineStore.getState().moveClip(clipId, startBeat, trackId)

  try {
    window.calliope?.dispatchCommand({
      command: 'clip:move',
      params: { clipId, startBeat, trackId },
    })
  } catch {
    // Engine clip commands not yet implemented
  }
}

/**
 * Resize a clip (change start beat and/or length).
 */
export function resizeClip(clipId: string, startBeat: number, lengthBeats: number): void {
  useTimelineStore.getState().resizeClip(clipId, startBeat, lengthBeats)

  try {
    window.calliope?.dispatchCommand({
      command: 'clip:resize',
      params: { clipId, startBeat, lengthBeats },
    })
  } catch {
    // Engine clip commands not yet implemented
  }
}

/**
 * Duplicate a clip (placed immediately after the original).
 */
export function duplicateClip(clipId: string): void {
  useTimelineStore.getState().duplicateClip(clipId)

  try {
    window.calliope?.dispatchCommand({
      command: 'clip:duplicate',
      params: { clipId },
    })
  } catch {
    // Engine clip commands not yet implemented
  }
}
