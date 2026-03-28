import { describe, it, expect, beforeEach } from 'vitest'
import { usePianoRollStore } from '../app/src/renderer/stores/piano-roll-store'
import { useTimelineStore } from '../app/src/renderer/stores/timeline-store'
import type { GridResolution } from '../app/src/renderer/types/timeline'
import {
  addNote,
  removeSelectedNotes,
  moveNotes,
  resizeNote,
  duplicateNotes,
  copyNotes,
  pasteNotes,
  cutNotes,
  quantizeSelectedNotes,
  scaleSelectedVelocities,
} from '../app/src/renderer/utils/note-operations'

describe('note-operations', () => {
  beforeEach(() => {
    usePianoRollStore.setState({
      activeClipId: null,
      notes: {},
      selectedNoteIds: new Set(),
      noteRowHeight: 16,
      scrollX: 0,
      scrollY: 1072,
      velocityLaneVisible: false,
      velocityLaneHeight: 80,
      panelHeight: 300,
      clipboard: [],
    })
    useTimelineStore.setState({
      gridResolution: 0.25 as GridResolution,
    })
  })

  describe('addNote', () => {
    it('creates a note in store with correct fields', () => {
      const id = addNote(60, 2, 1)
      const note = usePianoRollStore.getState().notes[id]
      expect(note).toBeDefined()
      expect(note.pitch).toBe(60)
      expect(note.startBeat).toBe(2)
      expect(note.lengthBeats).toBe(1)
      expect(note.velocity).toBe(100) // default
    })

    it('uses custom velocity when provided', () => {
      const id = addNote(64, 0, 0.5, 80)
      expect(usePianoRollStore.getState().notes[id].velocity).toBe(80)
    })
  })

  describe('removeSelectedNotes', () => {
    it('removes all selected notes', () => {
      const id1 = addNote(60, 0, 1)
      const id2 = addNote(64, 1, 1)
      usePianoRollStore.getState().selectNote(id1)
      usePianoRollStore.getState().selectNote(id2, true)
      removeSelectedNotes()
      expect(Object.keys(usePianoRollStore.getState().notes)).toHaveLength(0)
    })

    it('does nothing if nothing selected', () => {
      addNote(60, 0, 1)
      removeSelectedNotes()
      expect(Object.keys(usePianoRollStore.getState().notes)).toHaveLength(1)
    })
  })

  describe('moveNotes', () => {
    it('shifts notes by deltaBeat and deltaPitch', () => {
      const id = addNote(60, 2, 1)
      moveNotes([id], 1, 2)
      const note = usePianoRollStore.getState().notes[id]
      expect(note.pitch).toBe(62)
      expect(note.startBeat).toBe(3)
    })

    it('clamps pitch to 127', () => {
      const id = addNote(126, 0, 1)
      moveNotes([id], 0, 5)
      expect(usePianoRollStore.getState().notes[id].pitch).toBe(127)
    })

    it('clamps pitch to 0', () => {
      const id = addNote(2, 0, 1)
      moveNotes([id], 0, -5)
      expect(usePianoRollStore.getState().notes[id].pitch).toBe(0)
    })

    it('clamps startBeat to 0', () => {
      const id = addNote(60, 1, 1)
      moveNotes([id], -5, 0)
      expect(usePianoRollStore.getState().notes[id].startBeat).toBe(0)
    })
  })

  describe('resizeNote', () => {
    it('sets new length', () => {
      const id = addNote(60, 0, 1)
      resizeNote(id, 0.5)
      expect(usePianoRollStore.getState().notes[id].lengthBeats).toBe(0.5)
    })

    it('clamps to minimum grid resolution', () => {
      const id = addNote(60, 0, 1)
      resizeNote(id, 0.001)
      expect(usePianoRollStore.getState().notes[id].lengthBeats).toBe(0.25) // gridRes default
    })
  })

  describe('duplicateNotes', () => {
    it('duplicates selected notes offset by their length', () => {
      const id = addNote(60, 2, 1)
      usePianoRollStore.getState().selectNote(id)
      duplicateNotes()
      const notes = Object.values(usePianoRollStore.getState().notes)
      expect(notes).toHaveLength(2)
      const duplicate = notes.find((n) => n.id !== id)!
      expect(duplicate.startBeat).toBe(3) // 2 + 1
      expect(duplicate.pitch).toBe(60)
    })

    it('selects the new notes', () => {
      const id = addNote(60, 2, 1)
      usePianoRollStore.getState().selectNote(id)
      duplicateNotes()
      const selected = usePianoRollStore.getState().selectedNoteIds
      expect(selected.has(id)).toBe(false)
      expect(selected.size).toBe(1)
    })
  })

  describe('copyNotes and pasteNotes', () => {
    it('copies and pastes notes at new beat offset', () => {
      const id1 = addNote(60, 2, 1)
      const id2 = addNote(64, 3, 1)
      usePianoRollStore.getState().selectNote(id1)
      usePianoRollStore.getState().selectNote(id2, true)
      copyNotes()
      pasteNotes(8)
      const notes = Object.values(usePianoRollStore.getState().notes)
      expect(notes).toHaveLength(4)
      // Pasted notes should be at 8 and 9 (offset = 8 - 2 = 6)
      const pasted = notes.filter((n) => n.id !== id1 && n.id !== id2)
      const starts = pasted.map((n) => n.startBeat).sort()
      expect(starts).toEqual([8, 9])
    })

    it('selects the pasted notes', () => {
      const id1 = addNote(60, 2, 1)
      usePianoRollStore.getState().selectNote(id1)
      copyNotes()
      pasteNotes(8)
      const selected = usePianoRollStore.getState().selectedNoteIds
      expect(selected.has(id1)).toBe(false)
      expect(selected.size).toBe(1)
    })
  })

  describe('cutNotes', () => {
    it('copies to clipboard then removes', () => {
      const id = addNote(60, 2, 1)
      usePianoRollStore.getState().selectNote(id)
      cutNotes()
      expect(Object.keys(usePianoRollStore.getState().notes)).toHaveLength(0)
      expect(usePianoRollStore.getState().clipboard).toHaveLength(1)
    })
  })

  describe('quantizeSelectedNotes', () => {
    it('snaps to grid resolution', () => {
      const id = addNote(60, 0.3, 1)
      usePianoRollStore.getState().selectNote(id)
      quantizeSelectedNotes()
      expect(usePianoRollStore.getState().notes[id].startBeat).toBe(0.25)
    })

    it('snaps to triplet grid', () => {
      useTimelineStore.setState({ gridResolution: 0.16667 as GridResolution })
      const id = addNote(60, 0.2, 1)
      usePianoRollStore.getState().selectNote(id)
      quantizeSelectedNotes()
      expect(usePianoRollStore.getState().notes[id].startBeat).toBeCloseTo(0.16667, 4)
    })
  })

  describe('scaleSelectedVelocities', () => {
    it('scales proportionally', () => {
      const id1 = addNote(60, 0, 1, 100)
      const id2 = addNote(64, 1, 1, 80)
      usePianoRollStore.getState().selectNote(id1)
      usePianoRollStore.getState().selectNote(id2, true)
      scaleSelectedVelocities(id1, 50)
      expect(usePianoRollStore.getState().notes[id1].velocity).toBe(50)
      expect(usePianoRollStore.getState().notes[id2].velocity).toBe(40)
    })

    it('clamps to 127', () => {
      const id1 = addNote(60, 0, 1, 100)
      const id2 = addNote(64, 1, 1, 120)
      usePianoRollStore.getState().selectNote(id1)
      usePianoRollStore.getState().selectNote(id2, true)
      scaleSelectedVelocities(id1, 127)
      expect(usePianoRollStore.getState().notes[id2].velocity).toBe(127) // 120 * 1.27 = 152.4 -> clamped
    })

    it('clamps to 1', () => {
      const id1 = addNote(60, 0, 1, 100)
      const id2 = addNote(64, 1, 1, 5)
      usePianoRollStore.getState().selectNote(id1)
      usePianoRollStore.getState().selectNote(id2, true)
      scaleSelectedVelocities(id1, 1)
      expect(usePianoRollStore.getState().notes[id2].velocity).toBe(1) // 5 * 0.01 = 0.05 -> clamped to 1
    })
  })
})
