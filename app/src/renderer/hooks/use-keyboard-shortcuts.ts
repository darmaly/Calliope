import { useEffect } from 'react'
import { useTimelineStore } from '../stores/timeline-store'
import { usePianoRollStore } from '../stores/piano-roll-store'
import { useAppStore } from '../stores/app-store'
import { useProjectStore } from '../stores/project-store'
import { useMixerStore } from '../stores/mixer-store'
import { deleteClip, duplicateClip } from '../utils/clip-operations'
import {
  removeSelectedNotes,
  duplicateNotes,
  copyNotes,
  pasteNotes,
  cutNotes,
  quantizeSelectedNotes,
} from '../utils/note-operations'

export type FocusablePanel = 'timeline' | 'piano-roll' | 'mixer'

interface Modifiers {
  ctrl: boolean
  shift: boolean
  alt: boolean
}

/**
 * Pure function that maps a key + modifiers + focused panel to an action string.
 * Returns null if no action should be taken.
 */
export function routeShortcut(
  key: string,
  modifiers: Modifiers,
  focusedPanel: FocusablePanel,
): string | null {
  const { ctrl, shift } = modifiers

  // --- Global shortcuts (fire regardless of panel) ---

  // Space => play/toggle
  if (key === ' ' && !ctrl && !shift) return 'global:play-toggle'

  // Ctrl+Shift+Z => redo (check before Ctrl+Z)
  if (key === 'z' && ctrl && shift) return 'global:redo'

  // Ctrl+Z => undo
  if (key === 'z' && ctrl && !shift) return 'global:undo'

  // Ctrl+S => save
  if (key === 's' && ctrl && !shift) return 'global:save'

  // Ctrl+Shift+E => export
  if (key === 'e' && ctrl && shift) return 'global:export'

  // R => record toggle (no modifier)
  if (key === 'r' && !ctrl && !shift) return 'global:record-toggle'

  // L => loop toggle (no modifier)
  if (key === 'l' && !ctrl && !shift) return 'global:loop-toggle'

  // --- Panel-specific shortcuts ---

  if (focusedPanel === 'timeline') {
    if (key === 'Delete' || key === 'Backspace') return 'timeline:delete-selected'
    if (key === 'a' && ctrl) return 'timeline:select-all'
    if (key === 'd' && ctrl) return 'timeline:duplicate'
    if (key === 'Escape') return 'timeline:clear-selection'
    if (key === 's' && !ctrl && !shift) return 'timeline:toggle-snap'
  }

  if (focusedPanel === 'piano-roll') {
    if (key === 'Delete' || key === 'Backspace') return 'piano-roll:delete-selected'
    if (key === 'a' && ctrl) return 'piano-roll:select-all'
    if (key === 'd' && ctrl) return 'piano-roll:duplicate'
    if (key === 'c' && ctrl) return 'piano-roll:copy'
    if (key === 'v' && ctrl) return 'piano-roll:paste'
    if (key === 'x' && ctrl) return 'piano-roll:cut'
    if (key === 'Escape') return 'piano-roll:escape'
    if (key === 'q' && !ctrl && !shift) return 'piano-roll:quantize'
  }

  if (focusedPanel === 'mixer') {
    if (key === 'm' && !ctrl && !shift) return 'mixer:toggle-mute'
    if (key === 's' && !ctrl && !shift) return 'mixer:toggle-solo'
  }

  return null
}

/**
 * Unified keyboard shortcut hook.
 * Reads focusedPanel from app-store and routes shortcuts accordingly.
 * Registered as a capture-phase listener for priority over bubble-phase handlers.
 */
export function useKeyboardShortcuts(): void {
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      // Skip when typing in form elements
      const tag = (e.target as HTMLElement)?.tagName
      if (tag === 'INPUT' || tag === 'TEXTAREA' || (e.target as HTMLElement)?.isContentEditable) {
        return
      }

      const modifiers: Modifiers = {
        ctrl: e.metaKey || e.ctrlKey,
        shift: e.shiftKey,
        alt: e.altKey,
      }

      const focusedPanel = useAppStore.getState().focusedPanel
      const action = routeShortcut(e.key, modifiers, focusedPanel)

      if (action === null) return

      e.preventDefault()
      e.stopImmediatePropagation()

      // Dispatch action
      switch (action) {
        // --- Global ---
        case 'global:play-toggle': {
          const store = useTimelineStore.getState()
          if (store.isPlaying) {
            window.calliope?.transportStop()
          } else {
            window.calliope?.transportPlay()
          }
          break
        }
        case 'global:record-toggle':
          // Record toggle (engine API pending)
          break
        case 'global:loop-toggle':
          // Loop toggle (engine API pending)
          break
        case 'global:undo':
          window.calliope?.commandUndo()
          break
        case 'global:redo':
          window.calliope?.commandRedo()
          break
        case 'global:save':
          useProjectStore.getState().save()
          break
        case 'global:export':
          useProjectStore.getState().showExportDialog()
          break

        // --- Timeline ---
        case 'timeline:delete-selected': {
          const tStore = useTimelineStore.getState()
          if (tStore.selectedClipIds.size > 0) {
            for (const clipId of tStore.selectedClipIds) {
              deleteClip(clipId)
            }
            tStore.clearSelection()
          }
          break
        }
        case 'timeline:select-all': {
          const tStore = useTimelineStore.getState()
          tStore.selectClipsInRect(Object.keys(tStore.clips))
          break
        }
        case 'timeline:duplicate': {
          const tStore = useTimelineStore.getState()
          for (const clipId of tStore.selectedClipIds) {
            duplicateClip(clipId)
          }
          break
        }
        case 'timeline:clear-selection':
          useTimelineStore.getState().clearSelection()
          break
        case 'timeline:toggle-snap':
          useTimelineStore.getState().toggleSnap()
          break

        // --- Piano Roll ---
        case 'piano-roll:delete-selected': {
          const prState = usePianoRollStore.getState()
          if (prState.selectedNoteIds.size > 0) {
            removeSelectedNotes()
          }
          break
        }
        case 'piano-roll:select-all':
          usePianoRollStore.getState().selectAllNotes()
          break
        case 'piano-roll:duplicate':
          duplicateNotes()
          break
        case 'piano-roll:copy':
          copyNotes()
          break
        case 'piano-roll:paste': {
          const currentBeat = useTimelineStore.getState().currentBeat
          pasteNotes(currentBeat)
          break
        }
        case 'piano-roll:cut':
          cutNotes()
          break
        case 'piano-roll:escape': {
          const prState = usePianoRollStore.getState()
          if (prState.selectedNoteIds.size > 0) {
            prState.clearNoteSelection()
          } else {
            prState.setActiveClip(null)
          }
          break
        }
        case 'piano-roll:quantize':
          quantizeSelectedNotes()
          break

        // --- Mixer ---
        case 'mixer:toggle-mute': {
          const selectedTrackId = useTimelineStore.getState().selectedTrackId
          if (selectedTrackId) {
            useTimelineStore.getState().toggleMute(selectedTrackId)
          }
          break
        }
        case 'mixer:toggle-solo': {
          const selectedTrackId = useTimelineStore.getState().selectedTrackId
          if (selectedTrackId) {
            useTimelineStore.getState().toggleSolo(selectedTrackId)
          }
          break
        }
      }
    }

    // Capture phase for priority over bubble-phase listeners
    document.addEventListener('keydown', handleKeyDown, true)
    return () => document.removeEventListener('keydown', handleKeyDown, true)
  }, [])
}
