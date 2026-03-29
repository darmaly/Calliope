import { useEffect, useRef, useState, useCallback } from 'react'
import { Application, useApplication } from '@pixi/react'
import { useTimelineStore } from '../../stores/timeline-store'
import { usePianoRollStore } from '../../stores/piano-roll-store'
import { useShallow } from 'zustand/shallow'
import { PianoKeyboard, KEYBOARD_WIDTH } from './PianoKeyboard'
import { NoteGrid } from './NoteGrid'
import { NoteRect } from './NoteRect'
import { PianoRollPlayhead } from './PianoRollPlayhead'
import { pixelToBeat, snapToBeat, beatToPixel } from '../../utils/beat-math'
import {
  addNote,
  removeSelectedNotes,
  moveNotes,
  resizeNote,
  duplicateNotes,
  copyNotes,
  pasteNotes,
  cutNotes,
} from '../../utils/note-operations'
import { useContextMenu } from '../../hooks/use-context-menu'
import { ContextMenu } from '../shared/ContextMenu'

// Import pixi-setup to register base PixiJS components
import '../timeline/pixi-setup'

interface PianoRollCanvasProps {
  containerRef: React.RefObject<HTMLDivElement | null>
  trackColorHex: string
}

const MIN_ROW_HEIGHT = 4
const MAX_ROW_HEIGHT = 48
const EDGE_THRESHOLD = 6
const MIN_DRAG_DISTANCE = 4

type DragMode = 'none' | 'create' | 'select' | 'move' | 'resize-left' | 'resize-right'

interface DragState {
  mode: DragMode
  startX: number
  startY: number
  currentX: number
  currentY: number
  noteId?: string
  originals?: Record<string, { startBeat: number; pitch: number }>
}

/**
 * Inner PixiJS content rendered inside <Application>.
 */
function CanvasContent({ containerRef, trackColorHex }: PianoRollCanvasProps) {
  const app = useApplication()

  const { scrollX, scrollY, noteRowHeight, notes, selectedNoteIds } =
    usePianoRollStore(
      useShallow((s) => ({
        scrollX: s.scrollX,
        scrollY: s.scrollY,
        noteRowHeight: s.noteRowHeight,
        notes: s.notes,
        selectedNoteIds: s.selectedNoteIds,
      })),
    )

  const pixelsPerBeat = useTimelineStore((s) => s.pixelsPerBeat)

  const setScrollX = usePianoRollStore((s) => s.setScrollX)
  const setScrollY = usePianoRollStore((s) => s.setScrollY)
  const setNoteRowHeight = usePianoRollStore((s) => s.setNoteRowHeight)
  const setPixelsPerBeat = useTimelineStore((s) => s.setPixelsPerBeat)

  const [size, setSize] = useState({ width: 800, height: 400 })

  // Observe container size and force PixiJS renderer to resize
  useEffect(() => {
    const el = containerRef.current
    if (!el) return

    const observer = new ResizeObserver((entries) => {
      const entry = entries[0]
      if (entry) {
        const w = entry.contentRect.width
        const h = entry.contentRect.height
        setSize({ width: w, height: h })
        // Force PixiJS renderer to resize to match container
        const pixiApp = (app as any)?.app ?? app
        if (pixiApp?.renderer) {
          pixiApp.renderer.resize(w, h)
        }
      }
    })
    observer.observe(el)
    return () => observer.disconnect()
  }, [containerRef, app])

  // Imperatively sync scroll position for note grid container
  const scrollContainerRef = useRef<import('pixi.js').Container | null>(null)
  const keyboardContainerRef = useRef<import('pixi.js').Container | null>(null)

  useEffect(() => {
    const unsub = usePianoRollStore.subscribe((state) => {
      if (scrollContainerRef.current) {
        scrollContainerRef.current.x = -state.scrollX + KEYBOARD_WIDTH
        scrollContainerRef.current.y = -state.scrollY
      }
      if (keyboardContainerRef.current) {
        // Keyboard only scrolls vertically, stays at x=0
        keyboardContainerRef.current.y = -state.scrollY
      }
    })
    return unsub
  }, [])

  // Wheel handler for scroll/zoom
  useEffect(() => {
    const el = containerRef.current
    if (!el) return

    const handleWheel = (e: WheelEvent) => {
      e.preventDefault()

      const prState = usePianoRollStore.getState()
      const sx = prState.scrollX
      const sy = prState.scrollY
      const nrh = prState.noteRowHeight
      const ppb = useTimelineStore.getState().pixelsPerBeat

      if (e.altKey) {
        // Alt+wheel: adjust velocity of selected notes
        const selected = usePianoRollStore.getState().selectedNoteIds
        if (selected.size > 0) {
          const delta = e.deltaY > 0 ? -5 : 5
          for (const noteId of selected) {
            const note = usePianoRollStore.getState().notes[noteId]
            if (note) {
              const newVel = Math.max(1, Math.min(127, note.velocity + delta))
              usePianoRollStore.getState().updateNote(noteId, { velocity: newVel })
            }
          }
        }
      } else if ((e.ctrlKey || e.metaKey) && e.shiftKey) {
        // Ctrl+Shift+wheel: vertical zoom (note row height)
        const factor = e.deltaY > 0 ? 1 / 1.1 : 1.1
        const newH = Math.round(nrh * factor)
        setNoteRowHeight(Math.max(MIN_ROW_HEIGHT, Math.min(MAX_ROW_HEIGHT, newH)))
      } else if (e.ctrlKey || e.metaKey) {
        // Ctrl+wheel: horizontal zoom (pixelsPerBeat)
        const factor = e.deltaY > 0 ? 1 / 1.1 : 1.1
        const next = Math.min(Math.max(ppb * factor, 6), 192)
        setPixelsPerBeat(next)
      } else {
        // Normal scroll
        const dx = e.shiftKey ? e.deltaY : e.deltaX
        const dy = e.shiftKey ? 0 : e.deltaY

        // Horizontal scroll
        const maxScrollX = Math.max(0, 256 * ppb - (el.clientWidth ?? 800))
        setScrollX(Math.max(0, Math.min(maxScrollX, sx + dx)))

        // Vertical scroll: total height = 128 * noteRowHeight
        const totalH = 128 * nrh
        const maxScrollY = Math.max(0, totalH - (el.clientHeight ?? 400))
        setScrollY(Math.max(0, Math.min(maxScrollY, sy + dy)))
      }
    }

    el.addEventListener('wheel', handleWheel, { passive: false })
    return () => el.removeEventListener('wheel', handleWheel)
  }, [containerRef, setScrollX, setScrollY, setNoteRowHeight, setPixelsPerBeat])

  const noteList = Object.values(notes)

  return (
    <>
      {/* Keyboard column — only scrolls vertically */}
      <pixiContainer
        ref={(ref: import('pixi.js').Container | null) => {
          keyboardContainerRef.current = ref
        }}
        y={-scrollY}
      >
        <PianoKeyboard
          viewportHeight={size.height}
          scrollY={scrollY}
          noteRowHeight={noteRowHeight}
        />
      </pixiContainer>

      {/* Note grid + notes scroll container */}
      <pixiContainer
        ref={(ref: import('pixi.js').Container | null) => {
          scrollContainerRef.current = ref
        }}
        x={-scrollX + KEYBOARD_WIDTH}
        y={-scrollY}
      >
        <NoteGrid
          viewportWidth={size.width - KEYBOARD_WIDTH}
          viewportHeight={size.height}
          scrollX={scrollX}
          scrollY={scrollY}
        />

        {/* Note rectangles */}
        <pixiContainer cullable={true}>
          {noteList.map((note) => (
            <NoteRect
              key={note.id}
              note={note}
              pixelsPerBeat={pixelsPerBeat}
              noteRowHeight={noteRowHeight}
              selected={selectedNoteIds.has(note.id)}
              trackColorHex={trackColorHex}
            />
          ))}
        </pixiContainer>
      </pixiContainer>

    </>
  )
}

export function PianoRollCanvas({ containerRef, trackColorHex }: PianoRollCanvasProps) {
  const contextMenu = useContextMenu()

  const [drag, setDrag] = useState<DragState>({
    mode: 'none',
    startX: 0,
    startY: 0,
    currentX: 0,
    currentY: 0,
  })

  const [ghost, setGhost] = useState<{ x: number; y: number; width: number; height: number } | null>(null)
  const [selectionBox, setSelectionBox] = useState<{ x: number; y: number; width: number; height: number } | null>(null)

  const selectNote = usePianoRollStore((s) => s.selectNote)
  const toggleNoteSelection = usePianoRollStore((s) => s.toggleNoteSelection)
  const clearNoteSelection = usePianoRollStore((s) => s.clearNoteSelection)
  const selectAllNotes = usePianoRollStore((s) => s.selectAllNotes)
  const selectNotesInRect = usePianoRollStore((s) => s.selectNotesInRect)

  /**
   * Find a note at a given pixel position (relative to overlay).
   * Returns the note and which edge/body was hit, or null.
   */
  const findNoteAt = useCallback(
    (px: number, py: number): { noteId: string; edge: 'left' | 'right' | 'body' } | null => {
      const prState = usePianoRollStore.getState()
      const tlState = useTimelineStore.getState()
      const { notes, scrollX, scrollY, noteRowHeight } = prState
      const { pixelsPerBeat } = tlState

      // Subtract keyboard width before beat calculation
      const gridPx = px - KEYBOARD_WIDTH
      if (gridPx < 0) return null

      const beat = pixelToBeat(gridPx, pixelsPerBeat, scrollX)
      const pitch = 127 - Math.floor((py + scrollY) / noteRowHeight)

      if (pitch < 0 || pitch > 127) return null

      for (const note of Object.values(notes)) {
        if (note.pitch !== pitch) continue
        if (beat >= note.startBeat && beat <= note.startBeat + note.lengthBeats) {
          // Check edge proximity
          const leftEdgePx = beatToPixel(note.startBeat, pixelsPerBeat, scrollX) + KEYBOARD_WIDTH
          const rightEdgePx = beatToPixel(note.startBeat + note.lengthBeats, pixelsPerBeat, scrollX) + KEYBOARD_WIDTH

          let edge: 'left' | 'right' | 'body' = 'body'
          if (Math.abs(px - leftEdgePx) <= EDGE_THRESHOLD) edge = 'left'
          else if (Math.abs(px - rightEdgePx) <= EDGE_THRESHOLD) edge = 'right'

          return { noteId: note.id, edge }
        }
      }
      return null
    },
    [],
  )


  const handlePointerDown = useCallback(
    (e: React.PointerEvent<HTMLDivElement>) => {
      if (e.button === 2) return // Right-click handled by context menu

      const rect = (e.currentTarget as HTMLElement).getBoundingClientRect()
      const px = e.clientX - rect.left
      const py = e.clientY - rect.top

      const prState = usePianoRollStore.getState()

      // Note hit in the grid area
      {
        const hit = findNoteAt(px, py)

        if (hit) {
          const { noteId, edge } = hit

          // Selection logic
          if (e.shiftKey) {
            selectNote(noteId, true)
          } else if (e.ctrlKey || e.metaKey) {
            toggleNoteSelection(noteId)
          } else if (!prState.selectedNoteIds.has(noteId)) {
            selectNote(noteId)
          }

          if (edge === 'left') {
            setDrag({ mode: 'resize-left', startX: px, startY: py, currentX: px, currentY: py, noteId })
          } else if (edge === 'right') {
            setDrag({ mode: 'resize-right', startX: px, startY: py, currentX: px, currentY: py, noteId })
          } else {
            // Move mode — store originals for all selected notes
            const currentSelected = prState.selectedNoteIds.has(noteId)
              ? prState.selectedNoteIds
              : new Set([noteId])
            const originals: Record<string, { startBeat: number; pitch: number }> = {}
            for (const nid of currentSelected) {
              const note = prState.notes[nid]
              if (note) originals[nid] = { startBeat: note.startBeat, pitch: note.pitch }
            }
            setDrag({ mode: 'move', startX: px, startY: py, currentX: px, currentY: py, originals })
          }
        } else {
          // Click on empty grid
          clearNoteSelection()
          if (e.ctrlKey || e.metaKey) {
            // Ctrl+drag on empty: box select
            setDrag({ mode: 'select', startX: px, startY: py, currentX: px, currentY: py })
          } else {
            // Normal drag on empty: create note
            setDrag({ mode: 'create', startX: px, startY: py, currentX: px, currentY: py })
          }
        }
      }
      ;(e.currentTarget as HTMLElement).setPointerCapture(e.pointerId)
    },
    [findNoteAt, selectNote, toggleNoteSelection, clearNoteSelection],
  )

  const handlePointerMove = useCallback(
    (e: React.PointerEvent<HTMLDivElement>) => {
      const rect = (e.currentTarget as HTMLElement).getBoundingClientRect()
      const px = e.clientX - rect.left
      const py = e.clientY - rect.top

      const prState = usePianoRollStore.getState()
      const tlState = useTimelineStore.getState()
      const { scrollX, scrollY, noteRowHeight } = prState
      const { pixelsPerBeat, snapEnabled, gridResolution } = tlState

      if (drag.mode === 'none') {
        // Update cursor based on what's under the pointer
        const hit = findNoteAt(px, py)
        if (hit?.edge === 'left' || hit?.edge === 'right') {
          ;(e.currentTarget as HTMLElement).style.cursor = 'col-resize'
        } else if (hit) {
          ;(e.currentTarget as HTMLElement).style.cursor = 'pointer'
        } else {
          ;(e.currentTarget as HTMLElement).style.cursor = 'crosshair'
        }
        return
      }

      setDrag((prev) => ({ ...prev, currentX: px, currentY: py }))

      if (drag.mode === 'create') {
        const dist = Math.abs(px - drag.startX)
        if (dist < MIN_DRAG_DISTANCE) {
          setGhost(null)
          return
        }

        // Calculate ghost note position
        const gridStartPx = Math.min(drag.startX, px) - KEYBOARD_WIDTH
        const gridEndPx = Math.max(drag.startX, px) - KEYBOARD_WIDTH
        if (gridStartPx < 0 && gridEndPx < 0) return

        const startBeat = pixelToBeat(Math.max(0, gridStartPx), pixelsPerBeat, scrollX)
        const endBeat = pixelToBeat(Math.max(0, gridEndPx), pixelsPerBeat, scrollX)
        const snappedStart = snapEnabled ? snapToBeat(startBeat, gridResolution) : startBeat
        const snappedEnd = snapEnabled ? snapToBeat(endBeat, gridResolution) : endBeat
        const pitch = 127 - Math.floor((drag.startY + scrollY) / noteRowHeight)

        if (pitch >= 0 && pitch <= 127) {
          const ghostX = beatToPixel(snappedStart, pixelsPerBeat, scrollX) + KEYBOARD_WIDTH
          const ghostY = (127 - pitch) * noteRowHeight - scrollY
          const ghostWidth = (snappedEnd - snappedStart) * pixelsPerBeat

          setGhost({
            x: ghostX,
            y: ghostY,
            width: Math.max(ghostWidth, gridResolution * pixelsPerBeat),
            height: noteRowHeight,
          })
        }
      }

      if (drag.mode === 'select') {
        // Show rubber-band selection box
        const minX = Math.min(drag.startX, px)
        const minY = Math.min(drag.startY, py)
        const maxX = Math.max(drag.startX, px)
        const maxY = Math.max(drag.startY, py)
        setSelectionBox({ x: minX, y: minY, width: maxX - minX, height: maxY - minY })

        // Calculate intersecting notes
        const intersecting: string[] = []
        for (const note of Object.values(prState.notes)) {
          const noteLeft = beatToPixel(note.startBeat, pixelsPerBeat, scrollX) + KEYBOARD_WIDTH
          const noteRight = beatToPixel(note.startBeat + note.lengthBeats, pixelsPerBeat, scrollX) + KEYBOARD_WIDTH
          const noteTop = (127 - note.pitch) * noteRowHeight - scrollY
          const noteBottom = noteTop + noteRowHeight

          if (noteRight >= minX && noteLeft <= maxX && noteBottom >= minY && noteTop <= maxY) {
            intersecting.push(note.id)
          }
        }
        if (intersecting.length > 0) {
          selectNotesInRect(intersecting)
        }
      }

      if (drag.mode === 'move' && drag.originals) {
        const deltaPx = px - drag.startX
        let deltaBeat = deltaPx / pixelsPerBeat
        const deltaPitch = Math.round((drag.startY - py) / noteRowHeight)

        if (snapEnabled) {
          deltaBeat = snapToBeat(deltaBeat, gridResolution)
        }

        // Apply move to all selected notes using originals
        const noteIds = Object.keys(drag.originals)
        // Calculate updates from originals to avoid drift
        for (const nid of noteIds) {
          const orig = drag.originals[nid]
          if (!orig) continue
          const newPitch = Math.max(0, Math.min(127, orig.pitch + deltaPitch))
          const newStart = Math.max(0, orig.startBeat + deltaBeat)
          usePianoRollStore.getState().updateNote(nid, { pitch: newPitch, startBeat: newStart })
        }
      }

      if (drag.mode === 'resize-right' && drag.noteId) {
        const note = prState.notes[drag.noteId]
        if (note) {
          const gridPx = px - KEYBOARD_WIDTH
          let endBeat = pixelToBeat(gridPx, pixelsPerBeat, scrollX)
          if (snapEnabled) endBeat = snapToBeat(endBeat, gridResolution)
          const newLength = Math.max(gridResolution, endBeat - note.startBeat)
          resizeNote(drag.noteId, newLength)
        }
      }

      if (drag.mode === 'resize-left' && drag.noteId) {
        const note = prState.notes[drag.noteId]
        if (note) {
          const gridPx = px - KEYBOARD_WIDTH
          let newStart = pixelToBeat(gridPx, pixelsPerBeat, scrollX)
          if (snapEnabled) newStart = snapToBeat(newStart, gridResolution)
          const origEnd = note.startBeat + note.lengthBeats
          newStart = Math.min(newStart, origEnd - gridResolution)
          newStart = Math.max(0, newStart)
          const newLength = origEnd - newStart
          usePianoRollStore.getState().updateNote(drag.noteId, { startBeat: newStart })
          resizeNote(drag.noteId, newLength)
        }
      }

    },
    [drag, findNoteAt, selectNotesInRect],
  )

  const handlePointerUp = useCallback(
    (e: React.PointerEvent<HTMLDivElement>) => {
      const rect = (e.currentTarget as HTMLElement).getBoundingClientRect()
      const px = e.clientX - rect.left
      const py = e.clientY - rect.top

      const prState = usePianoRollStore.getState()
      const tlState = useTimelineStore.getState()
      const { scrollX, scrollY, noteRowHeight } = prState
      const { pixelsPerBeat, snapEnabled, gridResolution } = tlState

      if (drag.mode === 'create') {
        const dist = Math.abs(px - drag.startX)
        if (dist >= MIN_DRAG_DISTANCE) {
          // Calculate snapped start, pitch, length
          const gridStartPx = Math.min(drag.startX, px) - KEYBOARD_WIDTH
          const gridEndPx = Math.max(drag.startX, px) - KEYBOARD_WIDTH

          const startBeat = pixelToBeat(Math.max(0, gridStartPx), pixelsPerBeat, scrollX)
          const endBeat = pixelToBeat(Math.max(0, gridEndPx), pixelsPerBeat, scrollX)
          const snappedStart = snapEnabled ? snapToBeat(startBeat, gridResolution) : startBeat
          const snappedEnd = snapEnabled ? snapToBeat(endBeat, gridResolution) : endBeat
          const length = snappedEnd - snappedStart
          const pitch = 127 - Math.floor((drag.startY + scrollY) / noteRowHeight)

          if (length >= gridResolution && pitch >= 0 && pitch <= 127) {
            addNote(pitch, snappedStart, length)
          }
        }
        // If dist < MIN_DRAG_DISTANCE: treated as deselect (already cleared in pointerDown)
      }

      // Reset all transient state
      setDrag({ mode: 'none', startX: 0, startY: 0, currentX: 0, currentY: 0 })
      setGhost(null)
      setSelectionBox(null)
      ;(e.currentTarget as HTMLElement).releasePointerCapture(e.pointerId)
    },
    [drag],
  )

  const handleContextMenu = useCallback(
    (e: React.MouseEvent<HTMLDivElement>) => {
      e.preventDefault()
      // Suppress context menu on Ctrl+click (used for box select / toggle select)
      if (e.ctrlKey || e.metaKey) return

      const rect = (e.currentTarget as HTMLElement).getBoundingClientRect()
      const px = e.clientX - rect.left
      const py = e.clientY - rect.top
      const hit = findNoteAt(px, py)

      const prState = usePianoRollStore.getState()
      const tlState = useTimelineStore.getState()

      if (hit) {
        // Select the note if not already selected
        if (!prState.selectedNoteIds.has(hit.noteId)) {
          selectNote(hit.noteId)
        }
        contextMenu.show(e, [
          { label: 'Cut Notes', action: () => cutNotes() },
          { label: 'Copy Notes', action: () => copyNotes() },
          { label: 'Duplicate Notes', action: () => duplicateNotes() },
          { label: 'Delete Notes', action: () => removeSelectedNotes(), destructive: true },
        ])
      } else {
        // Empty grid context menu
        const gridPx = px - KEYBOARD_WIDTH
        const beat = gridPx > 0 ? pixelToBeat(gridPx, tlState.pixelsPerBeat, prState.scrollX) : 0
        const snappedBeat = tlState.snapEnabled ? snapToBeat(beat, tlState.gridResolution) : beat

        contextMenu.show(e, [
          {
            label: 'Paste Notes',
            action: () => pasteNotes(snappedBeat),
          },
          { label: 'Select All Notes', action: () => selectAllNotes() },
        ])
      }
    },
    [findNoteAt, selectNote, selectAllNotes, contextMenu],
  )

  return (
    <div className="relative w-full h-full">
      <Application resizeTo={containerRef} background="#1a1a2e" antialias>
        <CanvasContent containerRef={containerRef} trackColorHex={trackColorHex} />
      </Application>

      {/* Transparent DOM overlay for pointer events */}
      <div
        className="absolute inset-0"
        style={{ zIndex: 10 }}
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
        onContextMenu={handleContextMenu}
      />

      {/* Ghost note preview during creation */}
      {ghost && (
        <div
          className="absolute pointer-events-none rounded border border-[#6c63ff] bg-[#6c63ff]/20"
          style={{
            left: ghost.x,
            top: ghost.y,
            width: ghost.width,
            height: ghost.height,
            zIndex: 11,
            opacity: 0.5,
          }}
        />
      )}

      {/* Rubber-band selection box */}
      {selectionBox && (
        <div
          className="absolute pointer-events-none border border-[#6c63ff] bg-[#6c63ff]/10"
          style={{
            left: selectionBox.x,
            top: selectionBox.y,
            width: selectionBox.width,
            height: selectionBox.height,
            zIndex: 11,
          }}
        />
      )}

      {/* Playhead overlay */}
      <PianoRollPlayhead />

      {/* Context menu */}
      {contextMenu.visible && (
        <ContextMenu items={contextMenu.items} position={contextMenu.position} onClose={contextMenu.close} />
      )}
    </div>
  )
}
