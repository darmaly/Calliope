import { usePianoRollStore } from '../stores/piano-roll-store'
import { useTimelineStore } from '../stores/timeline-store'
import { snapToBeat } from './beat-math'
import type { MidiNote } from '../types/piano-roll'

export function addNote(
  pitch: number,
  startBeat: number,
  lengthBeats: number,
  velocity: number = 100,
): string {
  const id = crypto.randomUUID()
  const note: MidiNote = { id, pitch, startBeat, lengthBeats, velocity }
  usePianoRollStore.getState().addNote(note)
  return id
}

export function removeSelectedNotes(): void {
  const { selectedNoteIds } = usePianoRollStore.getState()
  if (selectedNoteIds.size === 0) return
  usePianoRollStore.getState().removeNotes([...selectedNoteIds])
}

export function moveNotes(noteIds: string[], deltaBeat: number, deltaPitch: number): void {
  const state = usePianoRollStore.getState()
  for (const noteId of noteIds) {
    const note = state.notes[noteId]
    if (!note) continue
    const newPitch = Math.max(0, Math.min(127, note.pitch + deltaPitch))
    const newStart = Math.max(0, note.startBeat + deltaBeat)
    state.updateNote(noteId, { pitch: newPitch, startBeat: newStart })
  }
}

export function resizeNote(noteId: string, newLengthBeats: number): void {
  const gridRes = useTimelineStore.getState().gridResolution
  const clamped = Math.max(gridRes, newLengthBeats)
  usePianoRollStore.getState().updateNote(noteId, { lengthBeats: clamped })
}

export function duplicateNotes(): void {
  const state = usePianoRollStore.getState()
  if (state.selectedNoteIds.size === 0) return
  const newIds: string[] = []
  for (const noteId of state.selectedNoteIds) {
    const note = state.notes[noteId]
    if (!note) continue
    const newId = crypto.randomUUID()
    const newNote: MidiNote = {
      ...note,
      id: newId,
      startBeat: note.startBeat + note.lengthBeats,
    }
    usePianoRollStore.getState().addNote(newNote)
    newIds.push(newId)
  }
  usePianoRollStore.getState().selectNotesInRect(newIds)
}

export function copyNotes(): void {
  const state = usePianoRollStore.getState()
  const copied = [...state.selectedNoteIds]
    .map((id) => state.notes[id])
    .filter(Boolean) as MidiNote[]
  state.setClipboard(copied)
}

export function pasteNotes(atBeat: number): void {
  const state = usePianoRollStore.getState()
  if (state.clipboard.length === 0) return
  const minBeat = Math.min(...state.clipboard.map((n) => n.startBeat))
  const offset = atBeat - minBeat
  const newIds: string[] = []
  for (const note of state.clipboard) {
    const newId = crypto.randomUUID()
    usePianoRollStore.getState().addNote({
      ...note,
      id: newId,
      startBeat: note.startBeat + offset,
    })
    newIds.push(newId)
  }
  usePianoRollStore.getState().selectNotesInRect(newIds)
}

export function cutNotes(): void {
  copyNotes()
  removeSelectedNotes()
}

export function quantizeSelectedNotes(): void {
  const prState = usePianoRollStore.getState()
  const gridRes = useTimelineStore.getState().gridResolution
  if (prState.selectedNoteIds.size === 0) return
  for (const noteId of prState.selectedNoteIds) {
    const note = prState.notes[noteId]
    if (!note) continue
    usePianoRollStore.getState().updateNote(noteId, {
      startBeat: snapToBeat(note.startBeat, gridRes),
    })
  }
}

export function scaleSelectedVelocities(draggedNoteId: string, newVelocity: number): void {
  const state = usePianoRollStore.getState()
  const original = state.notes[draggedNoteId]
  if (!original || original.velocity === 0) return
  const ratio = newVelocity / original.velocity
  for (const noteId of state.selectedNoteIds) {
    const note = state.notes[noteId]
    if (!note) continue
    const scaled = Math.max(1, Math.min(127, Math.round(note.velocity * ratio)))
    usePianoRollStore.getState().updateNote(noteId, { velocity: scaled })
  }
}
