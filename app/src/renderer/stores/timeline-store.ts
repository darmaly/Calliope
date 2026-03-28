import { create } from 'zustand'
import type {
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
  isPlaying: false,
  currentBeat: 0,
  bpm: 120,
  timeSignature: { numerator: 4, denominator: 4 },

  // Track actions
  addTrack: (name?: string, colorIndex?: number) =>
    set((state) => {
      const trackName = name ?? `Track ${state.tracks.length + 1}`
      const color = colorIndex ?? state.tracks.length % 8
      return {
        tracks: [
          ...state.tracks,
          {
            id: crypto.randomUUID(),
            name: trackName,
            colorIndex: color,
            muted: false,
            solo: false,
            armed: false,
            order: state.tracks.length,
          },
        ],
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
      return {
        tracks: state.tracks.filter((t) => t.id !== trackId),
        clips: newClips,
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
  addClip: (clip: Omit<Clip, 'id'>) =>
    set((state) => {
      const id = crypto.randomUUID()
      return {
        clips: { ...state.clips, [id]: { ...clip, id } as Clip },
      }
    }),

  removeClip: (clipId: string) =>
    set((state) => {
      const { [clipId]: _removed, ...rest } = state.clips
      return { clips: rest }
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

  duplicateClip: (clipId: string) =>
    set((state) => {
      const clip = state.clips[clipId]
      if (!clip) return state
      const newId = crypto.randomUUID()
      const newClip: Clip = {
        ...clip,
        id: newId,
        startBeat: clip.startBeat + clip.lengthBeats,
      }
      return { clips: { ...state.clips, [newId]: newClip } }
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
  setLoopRegion: (region: LoopRegion | null) => set(() => ({ loopRegion: region })),

  // Transport mirror
  setTransportState: (state) =>
    set(() => ({
      isPlaying: state.isPlaying,
      currentBeat: state.currentBeat,
      bpm: state.bpm,
      timeSignature: state.timeSignature,
    })),
}))
