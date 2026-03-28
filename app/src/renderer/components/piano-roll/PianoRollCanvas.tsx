import { useEffect, useRef, useState, useCallback } from 'react'
import { Application, useApplication } from '@pixi/react'
import { useTimelineStore } from '../../stores/timeline-store'
import { usePianoRollStore } from '../../stores/piano-roll-store'
import { useShallow } from 'zustand/shallow'
import { PianoKeyboard, KEYBOARD_WIDTH } from './PianoKeyboard'
import { NoteGrid } from './NoteGrid'
import { NoteRect } from './NoteRect'
import { VelocityLane } from './VelocityLane'
import { PianoRollPlayhead } from './PianoRollPlayhead'

// Import pixi-setup to register base PixiJS components
import '../timeline/pixi-setup'

interface PianoRollCanvasProps {
  containerRef: React.RefObject<HTMLDivElement | null>
  trackColorHex: string
}

const MIN_ROW_HEIGHT = 8
const MAX_ROW_HEIGHT = 32

/**
 * Inner PixiJS content rendered inside <Application>.
 */
function CanvasContent({ containerRef, trackColorHex }: PianoRollCanvasProps) {
  const app = useApplication()

  const { scrollX, scrollY, noteRowHeight, notes, selectedNoteIds, velocityLaneVisible, velocityLaneHeight } =
    usePianoRollStore(
      useShallow((s) => ({
        scrollX: s.scrollX,
        scrollY: s.scrollY,
        noteRowHeight: s.noteRowHeight,
        notes: s.notes,
        selectedNoteIds: s.selectedNoteIds,
        velocityLaneVisible: s.velocityLaneVisible,
        velocityLaneHeight: s.velocityLaneHeight,
      })),
    )

  const pixelsPerBeat = useTimelineStore((s) => s.pixelsPerBeat)

  const setScrollX = usePianoRollStore((s) => s.setScrollX)
  const setScrollY = usePianoRollStore((s) => s.setScrollY)
  const setNoteRowHeight = usePianoRollStore((s) => s.setNoteRowHeight)
  const setPixelsPerBeat = useTimelineStore((s) => s.setPixelsPerBeat)

  const [size, setSize] = useState({ width: 800, height: 400 })

  // Observe container size
  useEffect(() => {
    const el = containerRef.current
    if (!el) return

    const observer = new ResizeObserver((entries) => {
      const entry = entries[0]
      if (entry) {
        setSize({
          width: entry.contentRect.width,
          height: entry.contentRect.height,
        })
      }
    })
    observer.observe(el)
    return () => observer.disconnect()
  }, [containerRef])

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

      if ((e.ctrlKey || e.metaKey) && e.shiftKey) {
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

  // Calculate area for velocity lane
  const noteAreaHeight = velocityLaneVisible
    ? size.height - velocityLaneHeight
    : size.height

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
          viewportHeight={noteAreaHeight}
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
          viewportHeight={noteAreaHeight}
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

      {/* Velocity lane — scrolls horizontally only */}
      {velocityLaneVisible && (
        <pixiContainer x={-scrollX + KEYBOARD_WIDTH} y={0}>
          <VelocityLane
            viewportWidth={size.width - KEYBOARD_WIDTH}
            laneHeight={velocityLaneHeight}
            scrollX={scrollX}
            trackColorHex={trackColorHex}
            laneY={noteAreaHeight}
          />
        </pixiContainer>
      )}
    </>
  )
}

export function PianoRollCanvas({ containerRef, trackColorHex }: PianoRollCanvasProps) {
  return (
    <div className="relative w-full h-full">
      <Application resizeTo={containerRef} background="#1a1a2e" antialias>
        <CanvasContent containerRef={containerRef} trackColorHex={trackColorHex} />
      </Application>

      {/* Transparent DOM overlay for pointer events — Plan 03 will add handlers */}
      <div className="absolute inset-0" style={{ zIndex: 10 }} />

      {/* Playhead overlay */}
      <PianoRollPlayhead />
    </div>
  )
}
