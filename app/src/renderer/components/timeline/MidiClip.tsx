import React, { useCallback } from 'react'
import type { Graphics } from 'pixi.js'
import type { Clip } from '../../types/timeline'
import { TRACK_COLORS } from '../../utils/colors'

export interface MidiClipProps {
  clip: Clip
  trackColorIndex: number
  pixelsPerBeat: number
  isSelected: boolean
  trackRowY: number
}

const CLIP_HEIGHT = 72 // 80px track - 8px padding
const TOP_BAR_HEIGHT = 4
const BORDER_RADIUS = 4
const MIN_WIDTH = 8
const ACCENT_COLOR = 0x6c63ff
const SELECTION_BORDER = 2

function hexStringToNumber(hex: string): number {
  return parseInt(hex.replace('#', ''), 16)
}

export const MidiClip: React.FC<MidiClipProps> = React.memo(
  ({ clip, trackColorIndex, pixelsPerBeat, isSelected, trackRowY }) => {
    const clipWidth = Math.max(clip.lengthBeats * pixelsPerBeat, MIN_WIDTH)
    const clipX = clip.startBeat * pixelsPerBeat
    const colorHex = hexStringToNumber(TRACK_COLORS[trackColorIndex % TRACK_COLORS.length])

    const draw = useCallback(
      (g: Graphics) => {
        g.clear()

        // Clip body — rounded rectangle with 30% opacity fill
        g.roundRect(0, 0, clipWidth, CLIP_HEIGHT, BORDER_RADIUS)
        g.fill({ color: colorHex, alpha: 0.3 })

        // Top color bar — full opacity
        g.roundRect(0, 0, clipWidth, TOP_BAR_HEIGHT, BORDER_RADIUS)
        g.fill({ color: colorHex, alpha: 1.0 })
        // Fill the bottom corners of the top bar (they shouldn't be rounded)
        g.rect(0, TOP_BAR_HEIGHT / 2, clipWidth, TOP_BAR_HEIGHT / 2)
        g.fill({ color: colorHex, alpha: 1.0 })

        // Selection border
        if (isSelected) {
          g.roundRect(0, 0, clipWidth, CLIP_HEIGHT, BORDER_RADIUS)
          g.stroke({ color: ACCENT_COLOR, width: SELECTION_BORDER })
        }
      },
      [clipWidth, colorHex, isSelected],
    )

    return (
      <pixiContainer
        x={clipX}
        y={trackRowY}
        eventMode="static"
        cursor="pointer"
        cullable={true}
      >
        <pixiGraphics draw={draw} />
        <pixiText
          text={clip.name}
          x={4}
          y={TOP_BAR_HEIGHT + 2}
          style={{
            fontSize: 11,
            fill: '#eeeeee',
            fontFamily: 'Inter, sans-serif',
          }}
        />
      </pixiContainer>
    )
  },
)

MidiClip.displayName = 'MidiClip'
