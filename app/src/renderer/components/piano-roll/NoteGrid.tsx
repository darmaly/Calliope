import { useCallback } from 'react'
import { useTimelineStore } from '../../stores/timeline-store'
import { usePianoRollStore } from '../../stores/piano-roll-store'
import { useShallow } from 'zustand/shallow'
import type { Graphics } from 'pixi.js'

interface NoteGridProps {
  viewportWidth: number
  viewportHeight: number
  scrollX: number
  scrollY: number
}

export function NoteGrid({ viewportWidth, viewportHeight, scrollX, scrollY }: NoteGridProps) {
  const { pixelsPerBeat, gridResolution, timeSignature } = useTimelineStore(
    useShallow((s) => ({
      pixelsPerBeat: s.pixelsPerBeat,
      gridResolution: s.gridResolution,
      timeSignature: s.timeSignature,
    })),
  )
  const noteRowHeight = usePianoRollStore((s) => s.noteRowHeight)

  const beatsPerBar = timeSignature.numerator
  const totalHeight = 128 * noteRowHeight

  const draw = useCallback(
    (g: Graphics) => {
      g.clear()
      if (viewportWidth <= 0 || viewportHeight <= 0) return

      // --- Horizontal pitch rows ---
      const topPitch = Math.min(127, 127 - Math.floor(scrollY / noteRowHeight) + 1)
      const bottomPitch = Math.max(0, 127 - Math.ceil((scrollY + viewportHeight) / noteRowHeight) - 1)

      for (let pitch = bottomPitch; pitch <= topPitch; pitch++) {
        const y = (127 - pitch) * noteRowHeight

        // C-note octave highlight
        if (pitch % 12 === 0) {
          g.rect(scrollX, y, viewportWidth, noteRowHeight)
          g.fill({ color: 0xffffff, alpha: 0.04 })
        }

        // Pitch row border
        g.moveTo(scrollX, y + noteRowHeight)
        g.lineTo(scrollX + viewportWidth, y + noteRowHeight)
        g.stroke({ width: 1, color: 0xffffff, alpha: 0.05 })
      }

      // --- Vertical beat columns ---
      const startBeat = Math.max(0, scrollX / pixelsPerBeat)
      const endBeat = (scrollX + viewportWidth) / pixelsPerBeat

      // Bar boundaries
      const startBar = Math.floor(startBeat / beatsPerBar)
      const endBar = Math.ceil(endBeat / beatsPerBar) + 1

      for (let bar = startBar; bar <= endBar; bar++) {
        const beat = bar * beatsPerBar
        const x = beat * pixelsPerBeat
        g.moveTo(x, scrollY)
        g.lineTo(x, scrollY + viewportHeight)
        g.stroke({ width: 1, color: 0xffffff, alpha: 0.12 })
      }

      // Grid subdivisions
      const startGrid = Math.floor(startBeat / gridResolution) * gridResolution
      for (let beat = startGrid; beat <= endBeat; beat += gridResolution) {
        if (Math.abs(beat % beatsPerBar) < 0.001) continue
        const x = beat * pixelsPerBeat
        g.moveTo(x, scrollY)
        g.lineTo(x, scrollY + viewportHeight)
        g.stroke({ width: 1, color: 0xffffff, alpha: 0.05 })
      }
    },
    [
      pixelsPerBeat,
      gridResolution,
      beatsPerBar,
      noteRowHeight,
      viewportWidth,
      viewportHeight,
      scrollX,
      scrollY,
    ],
  )

  return <pixiGraphics draw={draw} />
}
