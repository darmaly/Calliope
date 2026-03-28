import { useCallback } from 'react'
import { useTimelineStore } from '../../stores/timeline-store'
import { useShallow } from 'zustand/shallow'
import type { Graphics } from 'pixi.js'

interface GridLayerProps {
  viewportWidth: number
  viewportHeight: number
  scrollX: number
}

export function GridLayer({ viewportWidth, viewportHeight, scrollX }: GridLayerProps) {
  const { pixelsPerBeat, gridResolution, timeSignature, tracks } = useTimelineStore(
    useShallow((s) => ({
      pixelsPerBeat: s.pixelsPerBeat,
      gridResolution: s.gridResolution,
      timeSignature: s.timeSignature,
      tracks: s.tracks,
    }))
  )

  const trackHeight = 80
  const beatsPerBar = timeSignature.numerator
  const totalHeight = Math.max(tracks.length * trackHeight, viewportHeight)

  const draw = useCallback(
    (g: Graphics) => {
      g.clear()

      if (viewportWidth <= 0 || viewportHeight <= 0) return

      // Calculate visible beat range
      const startBeat = Math.max(0, scrollX / pixelsPerBeat)
      const endBeat = (scrollX + viewportWidth) / pixelsPerBeat

      // --- Vertical grid lines ---

      // Major lines: bar boundaries
      const startBar = Math.floor(startBeat / beatsPerBar)
      const endBar = Math.ceil(endBeat / beatsPerBar) + 1

      for (let bar = startBar; bar <= endBar; bar++) {
        const beat = bar * beatsPerBar
        const x = beat * pixelsPerBeat
        g.moveTo(x, 0)
        g.lineTo(x, totalHeight)
        g.stroke({ width: 1, color: 0xffffff, alpha: 0.12 })
      }

      // Minor lines: grid resolution subdivisions
      const gridBeats = gridResolution * beatsPerBar // resolution is fraction of a beat
      const startGrid = Math.floor(startBeat / gridResolution) * gridResolution
      for (let beat = startGrid; beat <= endBeat; beat += gridResolution) {
        // Skip if this is already a bar boundary
        if (Math.abs(beat % beatsPerBar) < 0.001) continue
        const x = beat * pixelsPerBeat
        g.moveTo(x, 0)
        g.lineTo(x, totalHeight)
        g.stroke({ width: 1, color: 0xffffff, alpha: 0.05 })
      }

      // --- Horizontal track divider lines ---
      for (let i = 1; i <= tracks.length; i++) {
        const y = i * trackHeight
        g.moveTo(scrollX, y)
        g.lineTo(scrollX + viewportWidth, y)
        g.stroke({ width: 1, color: 0xffffff, alpha: 0.2 })
      }
    },
    [pixelsPerBeat, gridResolution, timeSignature, tracks.length, viewportWidth, viewportHeight, scrollX, beatsPerBar, totalHeight]
  )

  return <pixiGraphics draw={draw} />
}
