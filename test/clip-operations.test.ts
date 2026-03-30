import { describe, it, expect, beforeEach, vi } from 'vitest'

// Mock window.calliope for engine clip bridge calls (not available in test env)
;(globalThis as any).window = {
  calliope: {
    clipAdd: vi.fn().mockResolvedValue(true),
    clipRemove: vi.fn().mockResolvedValue(true),
    clipUpdate: vi.fn().mockResolvedValue(true),
    clipClear: vi.fn().mockResolvedValue(true),
  },
}
import { useTimelineStore } from '../app/src/renderer/stores/timeline-store'
import {
  createClip,
  deleteClip,
  moveClip,
  resizeClip,
  duplicateClip,
} from '../app/src/renderer/utils/clip-operations'

describe('clip-operations', () => {
  let trackId: string

  beforeEach(() => {
    useTimelineStore.setState({
      tracks: [],
      clips: {},
      scrollX: 0,
      scrollY: 0,
      pixelsPerBeat: 24,
      gridResolution: 0.25,
      snapEnabled: true,
      selectedClipIds: new Set(),
      selectedTrackId: null,
      loopRegion: null,
      isPlaying: false,
      currentBeat: 0,
      bpm: 120,
      timeSignature: { numerator: 4, denominator: 4 },
    })

    // Add a track for clips
    useTimelineStore.getState().addTrack('Test Track')
    trackId = useTimelineStore.getState().tracks[0].id
  })

  describe('createClip', () => {
    it('creates a MIDI clip via store addClip', () => {
      createClip(trackId, 0, 4, 'midi')
      const clips = Object.values(useTimelineStore.getState().clips)
      expect(clips).toHaveLength(1)
      expect(clips[0].trackId).toBe(trackId)
      expect(clips[0].type).toBe('midi')
      expect(clips[0].startBeat).toBe(0)
      expect(clips[0].lengthBeats).toBe(4)
      expect(clips[0].name).toBe('MIDI Clip')
    })

    it('creates an audio clip via store addClip', () => {
      createClip(trackId, 8, 2, 'audio')
      const clips = Object.values(useTimelineStore.getState().clips)
      expect(clips).toHaveLength(1)
      expect(clips[0].type).toBe('audio')
      expect(clips[0].name).toBe('Audio Clip')
    })
  })

  describe('deleteClip', () => {
    it('removes a clip from the store', () => {
      createClip(trackId, 0, 4, 'midi')
      const clipId = Object.keys(useTimelineStore.getState().clips)[0]
      deleteClip(clipId)
      expect(Object.keys(useTimelineStore.getState().clips)).toHaveLength(0)
    })
  })

  describe('moveClip', () => {
    it('moves a clip to a new startBeat', () => {
      createClip(trackId, 0, 4, 'midi')
      const clipId = Object.keys(useTimelineStore.getState().clips)[0]
      moveClip(clipId, 8)
      expect(useTimelineStore.getState().clips[clipId].startBeat).toBe(8)
    })

    it('moves a clip to a different track', () => {
      useTimelineStore.getState().addTrack('Track 2')
      const otherTrackId = useTimelineStore.getState().tracks[1].id

      createClip(trackId, 0, 4, 'midi')
      const clipId = Object.keys(useTimelineStore.getState().clips)[0]
      moveClip(clipId, 4, otherTrackId)
      expect(useTimelineStore.getState().clips[clipId].trackId).toBe(otherTrackId)
      expect(useTimelineStore.getState().clips[clipId].startBeat).toBe(4)
    })
  })

  describe('resizeClip', () => {
    it('resizes a clip', () => {
      createClip(trackId, 0, 4, 'midi')
      const clipId = Object.keys(useTimelineStore.getState().clips)[0]
      resizeClip(clipId, 2, 8)
      expect(useTimelineStore.getState().clips[clipId].startBeat).toBe(2)
      expect(useTimelineStore.getState().clips[clipId].lengthBeats).toBe(8)
    })
  })

  describe('duplicateClip', () => {
    it('duplicates a clip offset by its length', () => {
      createClip(trackId, 0, 4, 'midi')
      const clipId = Object.keys(useTimelineStore.getState().clips)[0]
      duplicateClip(clipId)
      const clips = Object.values(useTimelineStore.getState().clips)
      expect(clips).toHaveLength(2)
      const dup = clips.find((c) => c.id !== clipId)!
      expect(dup.startBeat).toBe(4)
      expect(dup.trackId).toBe(trackId)
    })
  })
})
