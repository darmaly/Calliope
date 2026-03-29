import { useRef, useEffect, useCallback, useState } from 'react'
import { usePianoRollStore } from '../../stores/piano-roll-store'
import { useTimelineStore } from '../../stores/timeline-store'
import { useShallow } from 'zustand/shallow'
import { pixelToBeat } from '../../utils/beat-math'
import { scaleSelectedVelocities } from '../../utils/note-operations'
import { KEYBOARD_WIDTH } from './PianoKeyboard'

interface VelocityLaneDomProps {
  height: number
  trackColorHex: string
}

const BACKGROUND_COLOR = '#1e1e36'
const BORDER_COLOR = '#3a3a5a'
const BAR_PADDING = 4

/**
 * DOM-rendered velocity lane with Canvas 2D.
 * Rendered as a fixed-height element below the PixiJS canvas, so it is
 * unaffected by panel resize (panel resize only changes the note grid area).
 */
export function VelocityLaneDom({ height, trackColorHex }: VelocityLaneDomProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null)
  const containerRef = useRef<HTMLDivElement>(null)

  const { notes, scrollX, selectedNoteIds } = usePianoRollStore(
    useShallow((s) => ({
      notes: s.notes,
      scrollX: s.scrollX,
      selectedNoteIds: s.selectedNoteIds,
    })),
  )
  const pixelsPerBeat = useTimelineStore((s) => s.pixelsPerBeat)
  const selectNote = usePianoRollStore((s) => s.selectNote)

  const [dragNoteId, setDragNoteId] = useState<string | null>(null)
  const [tooltip, setTooltip] = useState<{ x: number; y: number; value: number } | null>(null)

  /** Find which note's velocity bar is at pixel x */
  const findBarAt = useCallback(
    (px: number): string | null => {
      const gridPx = px - KEYBOARD_WIDTH
      if (gridPx < 0) return null

      const beat = pixelToBeat(gridPx, pixelsPerBeat, scrollX)

      for (const note of Object.values(notes)) {
        if (beat >= note.startBeat && beat <= note.startBeat + note.lengthBeats) {
          return note.id
        }
      }
      return null
    },
    [notes, pixelsPerBeat, scrollX],
  )

  const handlePointerDown = useCallback(
    (e: React.PointerEvent<HTMLDivElement>) => {
      const rect = (e.currentTarget as HTMLElement).getBoundingClientRect()
      const px = e.clientX - rect.left
      const noteId = findBarAt(px)
      if (!noteId) return

      // Ensure the note is selected
      if (!usePianoRollStore.getState().selectedNoteIds.has(noteId)) {
        selectNote(noteId)
      }

      setDragNoteId(noteId)
      ;(e.currentTarget as HTMLElement).setPointerCapture(e.pointerId)
    },
    [findBarAt, selectNote],
  )

  const handlePointerMove = useCallback(
    (e: React.PointerEvent<HTMLDivElement>) => {
      const rect = (e.currentTarget as HTMLElement).getBoundingClientRect()
      const px = e.clientX - rect.left
      const py = e.clientY - rect.top

      if (!dragNoteId) {
        // Cursor hint
        const barId = findBarAt(px)
        ;(e.currentTarget as HTMLElement).style.cursor = barId ? 'ns-resize' : 'default'
        return
      }

      // Calculate velocity from Y position (top = 127, bottom = 1)
      const newVelocity = Math.max(1, Math.min(127, Math.round((1 - py / height) * 127)))

      const prState = usePianoRollStore.getState()
      if (prState.selectedNoteIds.size > 1 && prState.selectedNoteIds.has(dragNoteId)) {
        scaleSelectedVelocities(dragNoteId, newVelocity)
      } else {
        usePianoRollStore.getState().updateNote(dragNoteId, { velocity: newVelocity })
      }

      setTooltip({ x: e.clientX - rect.left + 12, y: e.clientY - rect.top - 20, value: newVelocity })
    },
    [dragNoteId, findBarAt, height],
  )

  const handlePointerUp = useCallback(
    (e: React.PointerEvent<HTMLDivElement>) => {
      setDragNoteId(null)
      setTooltip(null)
      ;(e.currentTarget as HTMLElement).releasePointerCapture(e.pointerId)
    },
    [],
  )

  const draw = useCallback(() => {
    const canvas = canvasRef.current
    const container = containerRef.current
    if (!canvas || !container) return

    const dpr = window.devicePixelRatio || 1
    const width = container.clientWidth

    canvas.width = width * dpr
    canvas.height = height * dpr
    canvas.style.width = `${width}px`
    canvas.style.height = `${height}px`

    const ctx = canvas.getContext('2d')
    if (!ctx) return

    ctx.scale(dpr, dpr)

    // Background
    ctx.fillStyle = BACKGROUND_COLOR
    ctx.fillRect(0, 0, width, height)

    // Top border
    ctx.strokeStyle = BORDER_COLOR
    ctx.lineWidth = 1
    ctx.beginPath()
    ctx.moveTo(0, 0.5)
    ctx.lineTo(width, 0.5)
    ctx.stroke()

    // Draw velocity bars starting after the keyboard area
    const barAreaStart = KEYBOARD_WIDTH
    const usableHeight = height - BAR_PADDING

    const viewportStartBeat = scrollX / pixelsPerBeat
    const viewportEndBeat = (scrollX + width - KEYBOARD_WIDTH) / pixelsPerBeat

    // Parse track color for bar rendering
    const r = parseInt(trackColorHex.slice(1, 3), 16)
    const g = parseInt(trackColorHex.slice(3, 5), 16)
    const b = parseInt(trackColorHex.slice(5, 7), 16)

    for (const note of Object.values(notes)) {
      const noteEndBeat = note.startBeat + note.lengthBeats
      if (noteEndBeat < viewportStartBeat || note.startBeat > viewportEndBeat) continue

      const x = barAreaStart + (note.startBeat - viewportStartBeat) * pixelsPerBeat
      const barWidth = Math.max(note.lengthBeats * pixelsPerBeat, 2)
      const barHeight = (note.velocity / 127) * usableHeight
      const barY = height - barHeight

      // Bar fill
      ctx.fillStyle = `rgba(${r}, ${g}, ${b}, 0.7)`
      ctx.fillRect(x + 1, barY, barWidth - 2, barHeight)

      // Bar border for visual distinction
      ctx.strokeStyle = `rgba(${r}, ${g}, ${b}, 1.0)`
      ctx.lineWidth = 1
      ctx.strokeRect(x + 0.5, barY + 0.5, barWidth - 1, barHeight - 1)
    }
  }, [notes, scrollX, pixelsPerBeat, height, trackColorHex])

  // Redraw on state changes
  useEffect(() => {
    draw()
  }, [draw])

  // Redraw on container resize
  useEffect(() => {
    const el = containerRef.current
    if (!el) return
    const observer = new ResizeObserver(() => draw())
    observer.observe(el)
    return () => observer.disconnect()
  }, [draw])

  return (
    <div
      ref={containerRef}
      className="shrink-0 relative"
      style={{ height }}
      onPointerDown={handlePointerDown}
      onPointerMove={handlePointerMove}
      onPointerUp={handlePointerUp}
    >
      <canvas ref={canvasRef} className="absolute inset-0 pointer-events-none" />

      {/* Velocity tooltip */}
      {tooltip && (
        <span
          className="absolute pointer-events-none bg-[#252542] border border-[#3a3a5a] rounded px-2 py-0.5 text-[10px] text-[#eeeeee]"
          style={{
            left: tooltip.x,
            top: tooltip.y,
            zIndex: 12,
          }}
        >
          Vel: {tooltip.value}
        </span>
      )}
    </div>
  )
}
