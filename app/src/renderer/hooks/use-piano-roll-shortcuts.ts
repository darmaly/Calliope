import { useEffect } from 'react'
import { usePianoRollStore } from '../stores/piano-roll-store'
import { useTimelineStore } from '../stores/timeline-store'
import {
  removeSelectedNotes,
  duplicateNotes,
  copyNotes,
  pasteNotes,
  cutNotes,
  quantizeSelectedNotes,
} from '../utils/note-operations'

/**
 * Keyboard shortcuts for the piano roll editor.
 * Only active when a clip is open (activeClipId is set).
 * Overlapping shortcuts with the timeline handler are stopped via stopImmediatePropagation.
 */
export function usePianoRollShortcuts(): void {
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      // Skip when typing in form elements
      const tag = (e.target as HTMLElement)?.tagName
      if (tag === 'INPUT' || tag === 'TEXTAREA' || (e.target as HTMLElement)?.isContentEditable) {
        return
      }

      // Only handle when piano roll is active
      const prState = usePianoRollStore.getState()
      if (prState.activeClipId === null) return

      const mod = e.metaKey || e.ctrlKey

      // Delete / Backspace — delete selected notes
      if (e.key === 'Delete' || e.key === 'Backspace') {
        if (prState.selectedNoteIds.size > 0) {
          e.preventDefault()
          e.stopImmediatePropagation()
          removeSelectedNotes()
        }
        return
      }

      // Ctrl/Cmd + Q — quantize selected notes
      if (mod && e.key === 'q') {
        e.preventDefault()
        e.stopImmediatePropagation()
        quantizeSelectedNotes()
        return
      }

      // Ctrl/Cmd + D — duplicate selected notes
      if (mod && e.key === 'd') {
        e.preventDefault()
        e.stopImmediatePropagation()
        duplicateNotes()
        return
      }

      // Ctrl/Cmd + C — copy selected notes
      if (mod && e.key === 'c') {
        e.preventDefault()
        e.stopImmediatePropagation()
        copyNotes()
        return
      }

      // Ctrl/Cmd + V — paste notes at playhead position
      if (mod && e.key === 'v') {
        e.preventDefault()
        e.stopImmediatePropagation()
        const currentBeat = useTimelineStore.getState().currentBeat
        pasteNotes(currentBeat)
        return
      }

      // Ctrl/Cmd + X — cut selected notes
      if (mod && e.key === 'x') {
        e.preventDefault()
        e.stopImmediatePropagation()
        cutNotes()
        return
      }

      // Ctrl/Cmd + A — select all notes
      if (mod && e.key === 'a') {
        e.preventDefault()
        e.stopImmediatePropagation()
        usePianoRollStore.getState().selectAllNotes()
        return
      }

      // Ctrl/Cmd + Z — undo
      if (mod && !e.shiftKey && e.key === 'z') {
        e.preventDefault()
        e.stopImmediatePropagation()
        window.calliope?.commandUndo()
        return
      }

      // Ctrl/Cmd + Shift + Z — redo
      if (mod && e.shiftKey && e.key === 'z') {
        e.preventDefault()
        e.stopImmediatePropagation()
        window.calliope?.commandRedo()
        return
      }

      // Escape — deselect notes or close piano roll
      if (e.key === 'Escape') {
        e.preventDefault()
        e.stopImmediatePropagation()
        if (prState.selectedNoteIds.size > 0) {
          usePianoRollStore.getState().clearNoteSelection()
        } else {
          usePianoRollStore.getState().setActiveClip(null)
        }
        return
      }
    }

    // Add with capture: true to fire before the timeline shortcuts handler
    document.addEventListener('keydown', handleKeyDown, true)
    return () => document.removeEventListener('keydown', handleKeyDown, true)
  }, [])
}
