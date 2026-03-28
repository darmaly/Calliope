export type GridResolution = 0.25 | 0.125 | 0.0625 | 0.03125 | 0.16667 | 0.08333 | 0.04167

export interface Track {
  id: string
  name: string
  colorIndex: number // 0-7, indexes into TRACK_COLORS
  muted: boolean
  solo: boolean
  armed: boolean
  order: number
}

export interface Clip {
  id: string
  trackId: string
  type: 'midi' | 'audio'
  startBeat: number
  lengthBeats: number
  name: string
  peakData?: Float32Array // audio clips only
  notes?: Record<string, import('./piano-roll').MidiNote> // MIDI clips only
}

export interface LoopRegion {
  startBeat: number
  endBeat: number
}

export interface TimelineState {
  // Tracks
  tracks: Track[]
  // Clips
  clips: Record<string, Clip>
  // Viewport
  scrollX: number
  scrollY: number
  pixelsPerBeat: number
  // Grid
  gridResolution: GridResolution
  snapEnabled: boolean
  // Selection
  selectedClipIds: Set<string>
  selectedTrackId: string | null
  // Loop
  loopRegion: LoopRegion | null
  // Transport (mirrored from engine)
  isPlaying: boolean
  currentBeat: number
  bpm: number
  timeSignature: { numerator: number; denominator: number }
}

export interface TimelineActions {
  // Track actions
  addTrack: (name?: string, colorIndex?: number) => void
  removeTrack: (trackId: string) => void
  renameTrack: (trackId: string, name: string) => void
  reorderTrack: (trackId: string, newOrder: number) => void
  setTrackColor: (trackId: string, colorIndex: number) => void
  toggleMute: (trackId: string) => void
  toggleSolo: (trackId: string) => void
  toggleArm: (trackId: string) => void
  // Clip actions
  addClip: (clip: Omit<Clip, 'id'>) => void
  removeClip: (clipId: string) => void
  moveClip: (clipId: string, startBeat: number, trackId?: string) => void
  resizeClip: (clipId: string, startBeat: number, lengthBeats: number) => void
  duplicateClip: (clipId: string) => void
  // Clip notes (piano roll integration)
  updateClipNotes: (clipId: string, notes: Record<string, import('./piano-roll').MidiNote>) => void
  // Selection
  selectClip: (clipId: string, multi?: boolean) => void
  toggleClipSelection: (clipId: string) => void
  selectClipsInRect: (clipIds: string[]) => void
  clearSelection: () => void
  selectTrack: (trackId: string | null) => void
  // Viewport
  setScrollX: (x: number) => void
  setScrollY: (y: number) => void
  setPixelsPerBeat: (ppb: number) => void
  // Grid
  setGridResolution: (res: GridResolution) => void
  toggleSnap: () => void
  // Loop
  setLoopRegion: (region: LoopRegion | null) => void
  // Transport mirror
  setTransportState: (state: {
    isPlaying: boolean
    currentBeat: number
    bpm: number
    timeSignature: { numerator: number; denominator: number }
  }) => void
}
