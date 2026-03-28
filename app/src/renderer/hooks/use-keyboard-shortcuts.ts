import { useEffect } from 'react'
import { useTimelineStore } from '../stores/timeline-store'
import { deleteClip, duplicateClip } from '../utils/clip-operations'

/**
 * Global keyboard shortcut handler for timeline operations.
 * Ignores events when the user is typing in an input or textarea.
 */
export function useKeyboardShortcuts(): void {
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      // Skip when typing in form elements
      const tag = (e.target as HTMLElement)?.tagName
      if (tag === 'INPUT' || tag === 'TEXTAREA' || (e.target as HTMLElement)?.isContentEditable) {
        return
      }

      const mod = e.metaKey || e.ctrlKey
      const store = useTimelineStore.getState()

      // Delete / Backspace — delete selected clips
      if (e.key === 'Delete' || e.key === 'Backspace') {
        if (store.selectedClipIds.size > 0) {
          e.preventDefault()
          for (const clipId of store.selectedClipIds) {
            deleteClip(clipId)
          }
          store.clearSelection()
        }
        return
      }

      // Ctrl/Cmd + A — select all clips
      if (mod && e.key === 'a') {
        e.preventDefault()
        store.selectClipsInRect(Object.keys(store.clips))
        return
      }

      // Escape — clear selection
      if (e.key === 'Escape') {
        e.preventDefault()
        store.clearSelection()
        return
      }

      // Ctrl/Cmd + Z — undo
      if (mod && !e.shiftKey && e.key === 'z') {
        e.preventDefault()
        window.calliope?.commandUndo()
        return
      }

      // Ctrl/Cmd + Shift + Z — redo
      if (mod && e.shiftKey && e.key === 'z') {
        e.preventDefault()
        window.calliope?.commandRedo()
        return
      }

      // Ctrl/Cmd + D — duplicate selected clips
      if (mod && e.key === 'd') {
        e.preventDefault()
        for (const clipId of store.selectedClipIds) {
          duplicateClip(clipId)
        }
        return
      }

      // Space — toggle play/pause
      if (e.code === 'Space' && !mod) {
        e.preventDefault()
        if (store.isPlaying) {
          window.calliope?.transportStop()
        } else {
          window.calliope?.transportPlay()
        }
        return
      }

      // S (no modifier) — toggle snap
      if (e.key === 's' && !mod && !e.shiftKey && !e.altKey) {
        e.preventDefault()
        store.toggleSnap()
        return
      }

      // Ctrl/Cmd + = — zoom in
      if (mod && (e.key === '=' || e.key === '+')) {
        e.preventDefault()
        const next = Math.min(store.pixelsPerBeat * 1.5, 192)
        store.setPixelsPerBeat(next)
        return
      }

      // Ctrl/Cmd + - — zoom out
      if (mod && e.key === '-') {
        e.preventDefault()
        const next = Math.max(store.pixelsPerBeat / 1.5, 6)
        store.setPixelsPerBeat(next)
        return
      }
    }

    document.addEventListener('keydown', handleKeyDown)
    return () => document.removeEventListener('keydown', handleKeyDown)
  }, [])
}
