import { create } from 'zustand'
import type { PianoRollState, PianoRollActions, MidiNote } from '../types/piano-roll'
import { useTimelineStore } from './timeline-store'

let _savedPanelHeight = 300

export const usePianoRollStore = create<PianoRollState & PianoRollActions>((set, get) => ({
  // Default state
  activeClipId: null,
  notes: {},
  selectedNoteIds: new Set<string>(),
  noteRowHeight: 16,
  scrollX: 0,
  scrollY: 1072, // Centered around C4: (127 - 60) * 16
  velocityLaneVisible: false,
  velocityLaneHeight: 80,
  panelHeight: 300,
  clipboard: [],

  // Notes
  setNotes: (notes: Record<string, MidiNote>) => set(() => ({ notes })),

  addNote: (note: MidiNote) =>
    set((state) => ({
      notes: { ...state.notes, [note.id]: note },
    })),

  removeNotes: (noteIds: string[]) =>
    set((state) => {
      const newNotes = { ...state.notes }
      const newSelected = new Set(state.selectedNoteIds)
      for (const id of noteIds) {
        delete newNotes[id]
        newSelected.delete(id)
      }
      return { notes: newNotes, selectedNoteIds: newSelected }
    }),

  updateNote: (noteId: string, updates: Partial<MidiNote>) =>
    set((state) => {
      const note = state.notes[noteId]
      if (!note) return state
      return {
        notes: { ...state.notes, [noteId]: { ...note, ...updates } },
      }
    }),

  // Selection
  selectNote: (noteId: string, multi?: boolean) =>
    set((state) => {
      if (multi) {
        const newSet = new Set(state.selectedNoteIds)
        newSet.add(noteId)
        return { selectedNoteIds: newSet }
      }
      return { selectedNoteIds: new Set([noteId]) }
    }),

  toggleNoteSelection: (noteId: string) =>
    set((state) => {
      const newSet = new Set(state.selectedNoteIds)
      if (newSet.has(noteId)) {
        newSet.delete(noteId)
      } else {
        newSet.add(noteId)
      }
      return { selectedNoteIds: newSet }
    }),

  selectNotesInRect: (noteIds: string[]) =>
    set(() => ({ selectedNoteIds: new Set(noteIds) })),

  clearNoteSelection: () => set(() => ({ selectedNoteIds: new Set<string>() })),

  selectAllNotes: () =>
    set((state) => ({
      selectedNoteIds: new Set(Object.keys(state.notes)),
    })),

  // Viewport
  setScrollX: (x: number) => set(() => ({ scrollX: Math.max(0, x) })),
  setScrollY: (y: number) => set(() => ({ scrollY: Math.max(0, y) })),
  setNoteRowHeight: (h: number) =>
    set(() => ({ noteRowHeight: Math.max(8, Math.min(32, h)) })),

  // Velocity lane
  toggleVelocityLane: () =>
    set((state) => ({ velocityLaneVisible: !state.velocityLaneVisible })),
  setVelocityLaneHeight: (h: number) => set(() => ({ velocityLaneHeight: h })),

  // Panel
  setPanelHeight: (h: number) => set(() => ({ panelHeight: h })),
  setCollapsed: (collapsed: boolean) =>
    set((state) => {
      if (collapsed) {
        _savedPanelHeight = state.panelHeight
        return { panelHeight: 36 }
      }
      return { panelHeight: _savedPanelHeight }
    }),

  // Clipboard
  setClipboard: (notes: MidiNote[]) => set(() => ({ clipboard: notes })),

  // Active clip
  setActiveClip: (clipId: string | null) => {
    const state = get()
    // Flush current notes back to timeline clip
    if (state.activeClipId) {
      useTimelineStore
        .getState()
        .updateClipNotes(state.activeClipId, state.notes)
    }

    if (clipId === null) {
      set({
        activeClipId: null,
        notes: {},
        selectedNoteIds: new Set<string>(),
      })
      return
    }

    // Load notes from the target clip
    const clip = useTimelineStore.getState().clips[clipId]
    const notes = clip?.notes ?? {}
    set({
      activeClipId: clipId,
      notes,
      selectedNoteIds: new Set<string>(),
    })
  },
}))
