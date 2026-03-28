import React, { useCallback } from 'react'
import type { Graphics } from 'pixi.js'
import { useTimelineStore } from '../../stores/timeline-store'

export interface LoopRegionProps {
  /** Total height of all track rows (number of tracks * track height). */
  worldHeight: number
}

const ACCENT_COLOR = 0x6c63ff
const FILL_ALPHA = 0.15
const BRACKET_ALPHA = 0.8
const BRACKET_WIDTH = 2

export const LoopRegion: React.FC<LoopRegionProps> = React.memo(
  ({ worldHeight }) => {
    const loopRegion = useTimelineStore((s) => s.loopRegion)
    const pixelsPerBeat = useTimelineStore((s) => s.pixelsPerBeat)

    const draw = useCallback(
      (g: Graphics) => {
        g.clear()
        if (!loopRegion) return

        const startX = loopRegion.startBeat * pixelsPerBeat
        const endX = loopRegion.endBeat * pixelsPerBeat
        const width = endX - startX

        // Tinted overlay fill at 15% opacity
        g.rect(startX, 0, width, worldHeight)
        g.fill({ color: ACCENT_COLOR, alpha: FILL_ALPHA })

        // Bracket line at start
        g.rect(startX, 0, BRACKET_WIDTH, worldHeight)
        g.fill({ color: ACCENT_COLOR, alpha: BRACKET_ALPHA })

        // Bracket line at end
        g.rect(endX - BRACKET_WIDTH, 0, BRACKET_WIDTH, worldHeight)
        g.fill({ color: ACCENT_COLOR, alpha: BRACKET_ALPHA })
      },
      [loopRegion, pixelsPerBeat, worldHeight],
    )

    if (!loopRegion) return null

    return <pixiGraphics draw={draw} />
  },
)

LoopRegion.displayName = 'LoopRegion'
