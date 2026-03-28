import { describe, it, expect, beforeEach } from 'vitest'
import { useTimelineStore } from '../app/src/renderer/stores/timeline-store'

describe('timeline store', () => {
  beforeEach(() => {
    // Reset store to initial state before each test
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
  })

  describe('initial state', () => {
    it('has empty tracks', () => {
      expect(useTimelineStore.getState().tracks).toEqual([])
    })

    it('has empty clips', () => {
      expect(useTimelineStore.getState().clips).toEqual({})
    })

    it('has default pixelsPerBeat of 24', () => {
      expect(useTimelineStore.getState().pixelsPerBeat).toBe(24)
    })

    it('has default gridResolution of 0.25', () => {
      expect(useTimelineStore.getState().gridResolution).toBe(0.25)
    })

    it('has snapEnabled true', () => {
      expect(useTimelineStore.getState().snapEnabled).toBe(true)
    })
  })

  describe('track actions', () => {
    it('addTrack() creates track with auto-incremented name', () => {
      useTimelineStore.getState().addTrack()
      expect(useTimelineStore.getState().tracks).toHaveLength(1)
      expect(useTimelineStore.getState().tracks[0].name).toBe('Track 1')

      useTimelineStore.getState().addTrack()
      expect(useTimelineStore.getState().tracks).toHaveLength(2)
      expect(useTimelineStore.getState().tracks[1].name).toBe('Track 2')
    })

    it('addTrack("Lead") creates track with given name', () => {
      useTimelineStore.getState().addTrack('Lead')
      expect(useTimelineStore.getState().tracks[0].name).toBe('Lead')
    })

    it('removeTrack removes track and its clips', () => {
      useTimelineStore.getState().addTrack()
      const trackId = useTimelineStore.getState().tracks[0].id

      useTimelineStore.getState().addClip({
        trackId,
        type: 'midi',
        startBeat: 0,
        lengthBeats: 4,
        name: 'Clip 1',
      })

      expect(Object.keys(useTimelineStore.getState().clips)).toHaveLength(1)

      useTimelineStore.getState().removeTrack(trackId)
      expect(useTimelineStore.getState().tracks).toHaveLength(0)
      expect(Object.keys(useTimelineStore.getState().clips)).toHaveLength(0)
    })

    it('renameTrack changes track name', () => {
      useTimelineStore.getState().addTrack()
      const trackId = useTimelineStore.getState().tracks[0].id

      useTimelineStore.getState().renameTrack(trackId, 'Bass')
      expect(useTimelineStore.getState().tracks[0].name).toBe('Bass')
    })

    it('reorderTrack changes track order', () => {
      useTimelineStore.getState().addTrack()
      const trackId = useTimelineStore.getState().tracks[0].id

      useTimelineStore.getState().reorderTrack(trackId, 2)
      expect(useTimelineStore.getState().tracks[0].order).toBe(2)
    })

    it('setTrackColor changes colorIndex', () => {
      useTimelineStore.getState().addTrack()
      const trackId = useTimelineStore.getState().tracks[0].id

      useTimelineStore.getState().setTrackColor(trackId, 3)
      expect(useTimelineStore.getState().tracks[0].colorIndex).toBe(3)
    })

    it('toggleMute toggles muted state', () => {
      useTimelineStore.getState().addTrack()
      const trackId = useTimelineStore.getState().tracks[0].id

      expect(useTimelineStore.getState().tracks[0].muted).toBe(false)
      useTimelineStore.getState().toggleMute(trackId)
      expect(useTimelineStore.getState().tracks[0].muted).toBe(true)
      useTimelineStore.getState().toggleMute(trackId)
      expect(useTimelineStore.getState().tracks[0].muted).toBe(false)
    })

    it('toggleSolo toggles solo state', () => {
      useTimelineStore.getState().addTrack()
      const trackId = useTimelineStore.getState().tracks[0].id

      expect(useTimelineStore.getState().tracks[0].solo).toBe(false)
      useTimelineStore.getState().toggleSolo(trackId)
      expect(useTimelineStore.getState().tracks[0].solo).toBe(true)
    })
  })

  describe('clip actions', () => {
    let trackId: string

    beforeEach(() => {
      useTimelineStore.getState().addTrack()
      trackId = useTimelineStore.getState().tracks[0].id
    })

    it('addClip adds clip to store', () => {
      useTimelineStore.getState().addClip({
        trackId,
        type: 'midi',
        startBeat: 0,
        lengthBeats: 4,
        name: 'Clip 1',
      })

      const clips = useTimelineStore.getState().clips
      const clipIds = Object.keys(clips)
      expect(clipIds).toHaveLength(1)
      expect(clips[clipIds[0]].name).toBe('Clip 1')
      expect(clips[clipIds[0]].trackId).toBe(trackId)
    })

    it('removeClip removes clip from store', () => {
      useTimelineStore.getState().addClip({
        trackId,
        type: 'midi',
        startBeat: 0,
        lengthBeats: 4,
        name: 'Clip 1',
      })

      const clipId = Object.keys(useTimelineStore.getState().clips)[0]
      useTimelineStore.getState().removeClip(clipId)
      expect(Object.keys(useTimelineStore.getState().clips)).toHaveLength(0)
    })

    it('moveClip changes startBeat', () => {
      useTimelineStore.getState().addClip({
        trackId,
        type: 'midi',
        startBeat: 0,
        lengthBeats: 4,
        name: 'Clip 1',
      })

      const clipId = Object.keys(useTimelineStore.getState().clips)[0]
      useTimelineStore.getState().moveClip(clipId, 8)
      expect(useTimelineStore.getState().clips[clipId].startBeat).toBe(8)
    })

    it('moveClip with trackId changes track and startBeat', () => {
      useTimelineStore.getState().addTrack('Track 2')
      const otherTrackId = useTimelineStore.getState().tracks[1].id

      useTimelineStore.getState().addClip({
        trackId,
        type: 'midi',
        startBeat: 0,
        lengthBeats: 4,
        name: 'Clip 1',
      })

      const clipId = Object.keys(useTimelineStore.getState().clips)[0]
      useTimelineStore.getState().moveClip(clipId, 4, otherTrackId)
      expect(useTimelineStore.getState().clips[clipId].startBeat).toBe(4)
      expect(useTimelineStore.getState().clips[clipId].trackId).toBe(otherTrackId)
    })

    it('resizeClip changes startBeat and lengthBeats', () => {
      useTimelineStore.getState().addClip({
        trackId,
        type: 'midi',
        startBeat: 0,
        lengthBeats: 4,
        name: 'Clip 1',
      })

      const clipId = Object.keys(useTimelineStore.getState().clips)[0]
      useTimelineStore.getState().resizeClip(clipId, 2, 6)
      expect(useTimelineStore.getState().clips[clipId].startBeat).toBe(2)
      expect(useTimelineStore.getState().clips[clipId].lengthBeats).toBe(6)
    })

    it('duplicateClip creates new clip offset by lengthBeats', () => {
      useTimelineStore.getState().addClip({
        trackId,
        type: 'midi',
        startBeat: 0,
        lengthBeats: 4,
        name: 'Clip 1',
      })

      const clipId = Object.keys(useTimelineStore.getState().clips)[0]
      useTimelineStore.getState().duplicateClip(clipId)

      const clips = useTimelineStore.getState().clips
      const clipIds = Object.keys(clips)
      expect(clipIds).toHaveLength(2)

      const newClipId = clipIds.find((id) => id !== clipId)!
      expect(clips[newClipId].startBeat).toBe(4) // offset by lengthBeats
      expect(clips[newClipId].trackId).toBe(trackId)
      expect(clips[newClipId].name).toBe('Clip 1')
    })
  })

  describe('selection actions', () => {
    it('selectClip sets selectedClipIds to just that id', () => {
      useTimelineStore.getState().selectClip('clip-1')
      expect(useTimelineStore.getState().selectedClipIds).toEqual(new Set(['clip-1']))
    })

    it('selectClip with multi adds to selection', () => {
      useTimelineStore.getState().selectClip('clip-1')
      useTimelineStore.getState().selectClip('clip-2', true)
      expect(useTimelineStore.getState().selectedClipIds).toEqual(new Set(['clip-1', 'clip-2']))
    })

    it('toggleClipSelection toggles membership', () => {
      useTimelineStore.getState().selectClip('clip-1')
      useTimelineStore.getState().toggleClipSelection('clip-1')
      expect(useTimelineStore.getState().selectedClipIds).toEqual(new Set())

      useTimelineStore.getState().toggleClipSelection('clip-2')
      expect(useTimelineStore.getState().selectedClipIds).toEqual(new Set(['clip-2']))
    })

    it('clearSelection empties selectedClipIds', () => {
      useTimelineStore.getState().selectClip('clip-1')
      useTimelineStore.getState().clearSelection()
      expect(useTimelineStore.getState().selectedClipIds).toEqual(new Set())
    })
  })

  describe('grid actions', () => {
    it('setGridResolution changes gridResolution', () => {
      useTimelineStore.getState().setGridResolution(0.125)
      expect(useTimelineStore.getState().gridResolution).toBe(0.125)
    })

    it('toggleSnap toggles snapEnabled', () => {
      expect(useTimelineStore.getState().snapEnabled).toBe(true)
      useTimelineStore.getState().toggleSnap()
      expect(useTimelineStore.getState().snapEnabled).toBe(false)
      useTimelineStore.getState().toggleSnap()
      expect(useTimelineStore.getState().snapEnabled).toBe(true)
    })
  })

  describe('loop actions', () => {
    it('setLoopRegion sets loop region', () => {
      useTimelineStore.getState().setLoopRegion({ startBeat: 4, endBeat: 8 })
      expect(useTimelineStore.getState().loopRegion).toEqual({ startBeat: 4, endBeat: 8 })
    })

    it('setLoopRegion(null) clears loop region', () => {
      useTimelineStore.getState().setLoopRegion({ startBeat: 4, endBeat: 8 })
      useTimelineStore.getState().setLoopRegion(null)
      expect(useTimelineStore.getState().loopRegion).toBeNull()
    })
  })

  describe('viewport actions', () => {
    it('setPixelsPerBeat updates zoom level', () => {
      useTimelineStore.getState().setPixelsPerBeat(48)
      expect(useTimelineStore.getState().pixelsPerBeat).toBe(48)
    })
  })
})
