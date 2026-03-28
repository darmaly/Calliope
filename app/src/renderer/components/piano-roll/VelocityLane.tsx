import { useCallback } from 'react'
import { usePianoRollStore } from '../../stores/piano-roll-store'
import { useTimelineStore } from '../../stores/timeline-store'
import type { Graphics } from 'pixi.js'

interface VelocityLaneProps {
  viewportWidth: number
  laneHeight: number
  scrollX: number
  trackColorHex: string
  laneY: number
}

const BACKGROUND_COLOR = 0x1e1e36
const BORDER_COLOR = 0x3a3a5a
const BAR_PADDING = 4

function hexStringToNumber(hex: string): number {
  return parseInt(hex.replace('#', ''), 16)
}

export function VelocityLane({
  viewportWidth,
  laneHeight,
  scrollX,
  trackColorHex,
  laneY,
}: VelocityLaneProps) {
  const notes = usePianoRollStore((s) => s.notes)
  const pixelsPerBeat = useTimelineStore((s) => s.pixelsPerBeat)
  const colorNum = hexStringToNumber(trackColorHex)

  const draw = useCallback(
    (g: Graphics) => {
      g.clear()
      if (laneHeight <= 0 || viewportWidth <= 0) return

      // Background
      g.rect(scrollX, 0, viewportWidth, laneHeight)
      g.fill({ color: BACKGROUND_COLOR })

      // Top border
      g.moveTo(scrollX, 0)
      g.lineTo(scrollX + viewportWidth, 0)
      g.stroke({ width: 1, color: BORDER_COLOR })

      // Velocity bars
      const startBeat = scrollX / pixelsPerBeat
      const endBeat = (scrollX + viewportWidth) / pixelsPerBeat
      const usableHeight = laneHeight - BAR_PADDING

      for (const note of Object.values(notes)) {
        const noteEndBeat = note.startBeat + note.lengthBeats
        if (noteEndBeat < startBeat || note.startBeat > endBeat) continue

        const x = note.startBeat * pixelsPerBeat
        const barWidth = Math.max(note.lengthBeats * pixelsPerBeat, 2)
        const barHeight = (note.velocity / 127) * usableHeight
        const barY = laneHeight - barHeight

        g.rect(x, barY, barWidth, barHeight)
        g.fill({ color: colorNum, alpha: 0.8 })
      }
    },
    [notes, pixelsPerBeat, laneHeight, viewportWidth, scrollX, colorNum],
  )

  return (
    <pixiContainer y={laneY}>
      <pixiGraphics draw={draw} />
    </pixiContainer>
  )
}
