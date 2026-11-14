import { create } from 'zustand'
import { INSTRUMENTS, clampParam, findParam } from '../../shared/catalog'
import type {
  InstrumentType,
  TimelineState,
  TimelineActions,
  Clip,
  GridResolution,
  LoopRegion,
} from '../types/timeline'

export const useTimelineStore = create<TimelineState & TimelineActions>((set, get) => ({
  // Default state
  tracks: [],
  clips: {},
  scrollX: 0,
  scrollY: 0,
  pixelsPerBeat: 24,
  gridResolution: 0.25 as GridResolution,
  snapEnabled: true,
  selectedClipIds: new Set<string>(),
  selectedTrackId: null,
  loopRegion: null,
  loopEnabled: false,
  metronomeEnabled: false,
  isPlaying: false,
  currentBeat: 0,
  bpm: 120,
  timeSignature: { numerator: 4, denominator: 4 },

  // Track actions
  addTrack: (name?: string, colorIndex?: number, instrument: InstrumentType = 'polysynth') => {
    const id = crypto.randomUUID()
    set((state) => {
      const trackName = name ?? `Track ${state.tracks.length + 1}`
      const color = colorIndex ?? state.tracks.length % 8
      return {
        tracks: [
          ...state.tracks,
          {
            id,
            name: trackName,
            instrument,
            params: {},
            colorIndex: color,
            muted: false,
            solo: false,
            armed: false,
            order: state.tracks.length,
          },
        ],
      }
    })
    return id
  },

  setTrackParam: (trackId: string, name: string, value: number) =>
    set((state) => {
      const track = state.tracks.find((t) => t.id === trackId)
      if (!track) return state
      const spec = findParam(INSTRUMENTS[track.instrument].params, name)
      if (!spec) return state
      const clamped = clampParam(spec, value)
      if (track.params[name] === clamped) return state
      return {
        tracks: state.tracks.map((t) =>
          t.id === trackId ? { ...t, params: { ...t.params, [name]: clamped } } : t,
        ),
      }
    }),

  removeTrack: (trackId: string) =>
    set((state) => {
      const newClips: Record<string, Clip> = {}
      for (const [id, clip] of Object.entries(state.clips)) {
        if (clip.trackId !== trackId) {
          newClips[id] = clip
        }
      }
      const selectedClipIds = new Set(
        [...state.selectedClipIds].filter((id) => newClips[id] !== undefined),
      )
      return {
        tracks: state.tracks.filter((t) => t.id !== trackId),
        clips: newClips,
        selectedClipIds,
        selectedTrackId: state.selectedTrackId === trackId ? null : state.selectedTrackId,
      }
    }),

  renameTrack: (trackId: string, name: string) =>
    set((state) => ({
      tracks: state.tracks.map((t) => (t.id === trackId ? { ...t, name } : t)),
    })),

  reorderTrack: (trackId: string, newOrder: number) =>
    set((state) => ({
      tracks: state.tracks.map((t) => (t.id === trackId ? { ...t, order: newOrder } : t)),
    })),

  setTrackColor: (trackId: string, colorIndex: number) =>
    set((state) => ({
      tracks: state.tracks.map((t) => (t.id === trackId ? { ...t, colorIndex } : t)),
    })),

  toggleMute: (trackId: string) =>
    set((state) => ({
      tracks: state.tracks.map((t) => (t.id === trackId ? { ...t, muted: !t.muted } : t)),
    })),

  toggleSolo: (trackId: string) =>
    set((state) => ({
      tracks: state.tracks.map((t) => (t.id === trackId ? { ...t, solo: !t.solo } : t)),
    })),

  toggleArm: (trackId: string) =>
    set((state) => ({
      tracks: state.tracks.map((t) => (t.id === trackId ? { ...t, armed: !t.armed } : t)),
    })),

  // Clip actions
  addClip: (clip: Omit<Clip, 'id'>) => {
    const id = crypto.randomUUID()
    set((state) => ({
      clips: { ...state.clips, [id]: { ...clip, id } as Clip },
    }))
    return id
  },

  removeClip: (clipId: string) =>
    set((state) => {
      if (!(clipId in state.clips)) return state
      const { [clipId]: _removed, ...rest } = state.clips
      const selectedClipIds = new Set(state.selectedClipIds)
      selectedClipIds.delete(clipId)
      return { clips: rest, selectedClipIds }
    }),

  moveClip: (clipId: string, startBeat: number, trackId?: string) =>
    set((state) => {
      const clip = state.clips[clipId]
      if (!clip) return state
      const updated = { ...clip, startBeat }
      if (trackId !== undefined) {
        updated.trackId = trackId
      }
      return { clips: { ...state.clips, [clipId]: updated } }
    }),

  resizeClip: (clipId: string, startBeat: number, lengthBeats: number) =>
    set((state) => {
      const clip = state.clips[clipId]
      if (!clip) return state
      return {
        clips: { ...state.clips, [clipId]: { ...clip, startBeat, lengthBeats } },
      }
    }),

  duplicateClip: (clipId: string) => {
    const clip = get().clips[clipId]
    if (!clip) return null
    const newId = crypto.randomUUID()
    const newClip: Clip = {
      ...clip,
      id: newId,
      startBeat: clip.startBeat + clip.lengthBeats,
    }
    set((state) => ({ clips: { ...state.clips, [newId]: newClip } }))
    return newId
  },

  // Clip notes (piano roll integration)
  updateClipNotes: (clipId, notes) =>
    set((state) => {
      const clip = state.clips[clipId]
      if (!clip) return state
      return { clips: { ...state.clips, [clipId]: { ...clip, notes } } }
    }),

  // Selection actions
  selectClip: (clipId: string, multi?: boolean) =>
    set((state) => {
      if (multi) {
        const newSet = new Set(state.selectedClipIds)
        newSet.add(clipId)
        return { selectedClipIds: newSet }
      }
      return { selectedClipIds: new Set([clipId]) }
    }),

  toggleClipSelection: (clipId: string) =>
    set((state) => {
      const newSet = new Set(state.selectedClipIds)
      if (newSet.has(clipId)) {
        newSet.delete(clipId)
      } else {
        newSet.add(clipId)
      }
      return { selectedClipIds: newSet }
    }),

  selectClipsInRect: (clipIds: string[]) =>
    set(() => ({ selectedClipIds: new Set(clipIds) })),

  clearSelection: () =>
    set(() => ({ selectedClipIds: new Set<string>() })),

  selectTrack: (trackId: string | null) =>
    set(() => ({ selectedTrackId: trackId })),

  // Viewport actions
  setScrollX: (x: number) => set(() => ({ scrollX: x })),
  setScrollY: (y: number) => set(() => ({ scrollY: y })),
  setPixelsPerBeat: (ppb: number) => set(() => ({ pixelsPerBeat: ppb })),

  // Grid actions
  setGridResolution: (res: GridResolution) => set(() => ({ gridResolution: res })),
  toggleSnap: () => set((state) => ({ snapEnabled: !state.snapEnabled })),

  // Loop actions
  // Drawing a region switches looping on; clearing it switches looping off.
  setLoopRegion: (region: LoopRegion | null) =>
    set(() => ({ loopRegion: region, loopEnabled: region !== null })),
  setLoopEnabled: (enabled: boolean) => set(() => ({ loopEnabled: enabled })),

  // Metronome
  setMetronomeEnabled: (enabled: boolean) => set(() => ({ metronomeEnabled: enabled })),

  // Project tempo
  setBpm: (bpm: number) =>
    set((state) => {
      if (!Number.isFinite(bpm)) return state
      return { bpm: Math.min(300, Math.max(20, bpm)) }
    }),
  setTimeSignature: (numerator: number, denominator: number) =>
    set((state) => {
      if (!Number.isInteger(numerator) || numerator < 1 || numerator > 32) return state
      if (![1, 2, 4, 8, 16, 32].includes(denominator)) return state
      return { timeSignature: { numerator, denominator } }
    }),

  // Playback position (mirrored from the engine many times a second)
  setPlaybackPosition: (isPlaying: boolean, currentBeat: number) =>
    set((state) =>
      state.isPlaying === isPlaying && state.currentBeat === currentBeat
        ? state
        : { isPlaying, currentBeat },
    ),

  // Transport mirror
  setTransportState: (state) =>
    set(() => ({
      isPlaying: state.isPlaying,
      currentBeat: state.currentBeat,
      bpm: state.bpm,
      timeSignature: state.timeSignature,
    })),
}))
