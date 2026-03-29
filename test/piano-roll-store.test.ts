import { describe, it, expect, beforeEach } from 'vitest'
import { usePianoRollStore } from '../app/src/renderer/stores/piano-roll-store'
import { useTimelineStore } from '../app/src/renderer/stores/timeline-store'
import type { MidiNote } from '../app/src/renderer/types/piano-roll'

function makeNote(overrides: Partial<MidiNote> = {}): MidiNote {
  return {
    id: crypto.randomUUID(),
    pitch: 60,
    startBeat: 0,
    lengthBeats: 1,
    velocity: 100,
    ...overrides,
  }
}

describe('piano-roll-store', () => {
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
  })

  describe('note model', () => {
    it('stores a note with correct fields', () => {
      const note = makeNote({ id: 'n1', pitch: 60, startBeat: 0, lengthBeats: 1, velocity: 100 })
      usePianoRollStore.getState().addNote(note)
      const stored = usePianoRollStore.getState().notes['n1']
      expect(stored).toEqual(note)
    })
  })

  describe('selection', () => {
    it('selectNote sets selectedNoteIds to single id', () => {
      usePianoRollStore.getState().selectNote('n1')
      expect(usePianoRollStore.getState().selectedNoteIds).toEqual(new Set(['n1']))
    })

    it('selectNote with multi=true adds to set', () => {
      usePianoRollStore.getState().selectNote('n1')
      usePianoRollStore.getState().selectNote('n2', true)
      expect(usePianoRollStore.getState().selectedNoteIds).toEqual(new Set(['n1', 'n2']))
    })

    it('toggleNoteSelection removes if present', () => {
      usePianoRollStore.getState().selectNote('n1')
      usePianoRollStore.getState().toggleNoteSelection('n1')
      expect(usePianoRollStore.getState().selectedNoteIds.size).toBe(0)
    })

    it('toggleNoteSelection adds if absent', () => {
      usePianoRollStore.getState().toggleNoteSelection('n1')
      expect(usePianoRollStore.getState().selectedNoteIds).toEqual(new Set(['n1']))
    })

    it('clearNoteSelection empties set', () => {
      usePianoRollStore.getState().selectNote('n1')
      usePianoRollStore.getState().clearNoteSelection()
      expect(usePianoRollStore.getState().selectedNoteIds.size).toBe(0)
    })

    it('selectAllNotes selects all note ids', () => {
      const n1 = makeNote({ id: 'n1' })
      const n2 = makeNote({ id: 'n2' })
      usePianoRollStore.getState().addNote(n1)
      usePianoRollStore.getState().addNote(n2)
      usePianoRollStore.getState().selectAllNotes()
      expect(usePianoRollStore.getState().selectedNoteIds).toEqual(new Set(['n1', 'n2']))
    })

    it('selectNotesInRect replaces selection', () => {
      usePianoRollStore.getState().selectNote('old')
      usePianoRollStore.getState().selectNotesInRect(['a', 'b'])
      expect(usePianoRollStore.getState().selectedNoteIds).toEqual(new Set(['a', 'b']))
    })
  })

  describe('scroll', () => {
    it('setScrollX sets scrollX', () => {
      usePianoRollStore.getState().setScrollX(100)
      expect(usePianoRollStore.getState().scrollX).toBe(100)
    })

    it('setScrollY sets scrollY', () => {
      usePianoRollStore.getState().setScrollY(200)
      expect(usePianoRollStore.getState().scrollY).toBe(200)
    })

    it('setScrollX clamps to >= 0', () => {
      usePianoRollStore.getState().setScrollX(-50)
      expect(usePianoRollStore.getState().scrollX).toBe(0)
    })

    it('setNoteRowHeight sets noteRowHeight', () => {
      usePianoRollStore.getState().setNoteRowHeight(24)
      expect(usePianoRollStore.getState().noteRowHeight).toBe(24)
    })

    it('setNoteRowHeight clamps to 4-48', () => {
      usePianoRollStore.getState().setNoteRowHeight(2)
      expect(usePianoRollStore.getState().noteRowHeight).toBe(4)
      usePianoRollStore.getState().setNoteRowHeight(64)
      expect(usePianoRollStore.getState().noteRowHeight).toBe(48)
    })
  })

  describe('velocity lane', () => {
    it('toggleVelocityLane flips visibility', () => {
      expect(usePianoRollStore.getState().velocityLaneVisible).toBe(false)
      usePianoRollStore.getState().toggleVelocityLane()
      expect(usePianoRollStore.getState().velocityLaneVisible).toBe(true)
      usePianoRollStore.getState().toggleVelocityLane()
      expect(usePianoRollStore.getState().velocityLaneVisible).toBe(false)
    })

    it('setVelocityLaneHeight sets height', () => {
      usePianoRollStore.getState().setVelocityLaneHeight(120)
      expect(usePianoRollStore.getState().velocityLaneHeight).toBe(120)
    })
  })

  describe('panel', () => {
    it('setPanelHeight sets panelHeight', () => {
      usePianoRollStore.getState().setPanelHeight(400)
      expect(usePianoRollStore.getState().panelHeight).toBe(400)
    })
  })

  describe('clipboard', () => {
    it('setClipboard stores notes array', () => {
      const notes = [makeNote({ id: 'c1' }), makeNote({ id: 'c2' })]
      usePianoRollStore.getState().setClipboard(notes)
      expect(usePianoRollStore.getState().clipboard).toEqual(notes)
    })
  })

  describe('active clip', () => {
    it('setActiveClip loads notes from timeline clip', () => {
      // Set up a clip with notes in timeline store
      const clipId = 'clip-1'
      const noteData: Record<string, MidiNote> = {
        n1: makeNote({ id: 'n1', pitch: 64 }),
      }
      useTimelineStore.setState((state) => ({
        clips: {
          ...state.clips,
          [clipId]: {
            id: clipId,
            trackId: 't1',
            type: 'midi' as const,
            startBeat: 0,
            lengthBeats: 4,
            name: 'Test',
            notes: noteData,
          },
        },
      }))

      usePianoRollStore.getState().setActiveClip(clipId)
      expect(usePianoRollStore.getState().activeClipId).toBe(clipId)
      expect(usePianoRollStore.getState().notes).toEqual(noteData)
      expect(usePianoRollStore.getState().selectedNoteIds.size).toBe(0)
    })

    it('setActiveClip(null) flushes notes back to clip', () => {
      const clipId = 'clip-2'
      useTimelineStore.setState((state) => ({
        clips: {
          ...state.clips,
          [clipId]: {
            id: clipId,
            trackId: 't1',
            type: 'midi' as const,
            startBeat: 0,
            lengthBeats: 4,
            name: 'Test',
            notes: {},
          },
        },
      }))

      // Open the clip
      usePianoRollStore.getState().setActiveClip(clipId)
      // Add a note while editing
      const note = makeNote({ id: 'added' })
      usePianoRollStore.getState().addNote(note)
      // Close the clip (flush)
      usePianoRollStore.getState().setActiveClip(null)

      // Notes should be flushed back to timeline clip
      const clip = useTimelineStore.getState().clips[clipId]
      expect(clip?.notes).toHaveProperty('added')
      expect(usePianoRollStore.getState().activeClipId).toBeNull()
      expect(usePianoRollStore.getState().notes).toEqual({})
    })
  })

  describe('removeNotes', () => {
    it('removes notes and clears from selection', () => {
      const n1 = makeNote({ id: 'n1' })
      const n2 = makeNote({ id: 'n2' })
      usePianoRollStore.getState().addNote(n1)
      usePianoRollStore.getState().addNote(n2)
      usePianoRollStore.getState().selectNote('n1')
      usePianoRollStore.getState().selectNote('n2', true)
      usePianoRollStore.getState().removeNotes(['n1'])
      expect(usePianoRollStore.getState().notes).not.toHaveProperty('n1')
      expect(usePianoRollStore.getState().notes).toHaveProperty('n2')
      expect(usePianoRollStore.getState().selectedNoteIds.has('n1')).toBe(false)
    })
  })

  describe('updateNote', () => {
    it('merges partial update into existing note', () => {
      const n1 = makeNote({ id: 'n1', pitch: 60, velocity: 80 })
      usePianoRollStore.getState().addNote(n1)
      usePianoRollStore.getState().updateNote('n1', { velocity: 120 })
      expect(usePianoRollStore.getState().notes['n1'].velocity).toBe(120)
      expect(usePianoRollStore.getState().notes['n1'].pitch).toBe(60)
    })
  })
})
