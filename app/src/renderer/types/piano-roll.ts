export interface MidiNote {
  id: string
  pitch: number // 0-127 MIDI pitch
  startBeat: number // Position in beats
  lengthBeats: number // Duration in beats
  velocity: number // 1-127
}

export interface PianoRollState {
  activeClipId: string | null
  notes: Record<string, MidiNote>
  selectedNoteIds: Set<string>
  noteRowHeight: number // 8-32, default 16 (per D-03)
  scrollX: number
  scrollY: number // default centered around C4 area
  velocityLaneVisible: boolean
  velocityLaneHeight: number // default 80
  panelHeight: number // split panel height, default 300
  clipboard: MidiNote[]
}

export interface PianoRollActions {
  // Notes
  setNotes: (notes: Record<string, MidiNote>) => void
  addNote: (note: MidiNote) => void
  removeNotes: (noteIds: string[]) => void
  updateNote: (noteId: string, updates: Partial<MidiNote>) => void
  // Selection
  selectNote: (noteId: string, multi?: boolean) => void
  toggleNoteSelection: (noteId: string) => void
  selectNotesInRect: (noteIds: string[]) => void
  clearNoteSelection: () => void
  selectAllNotes: () => void
  // Viewport
  setScrollX: (x: number) => void
  setScrollY: (y: number) => void
  setNoteRowHeight: (h: number) => void
  // Velocity lane
  toggleVelocityLane: () => void
  setVelocityLaneHeight: (h: number) => void
  // Panel
  setPanelHeight: (h: number) => void
  setCollapsed: (collapsed: boolean) => void
  // Clipboard
  setClipboard: (notes: MidiNote[]) => void
  // Active clip
  setActiveClip: (clipId: string | null) => void
}
