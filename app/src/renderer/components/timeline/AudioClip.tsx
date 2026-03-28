import React, { useCallback } from 'react'
import type { Graphics } from 'pixi.js'
import type { Clip } from '../../types/timeline'
import { TRACK_COLORS } from '../../utils/colors'
import { peaksToVertices } from '../../utils/waveform'

export interface AudioClipProps {
  clip: Clip
  trackColorIndex: number
  pixelsPerBeat: number
  isSelected: boolean
  trackRowY: number
}

const CLIP_HEIGHT = 72
const TOP_BAR_HEIGHT = 4
const BORDER_RADIUS = 4
const MIN_WIDTH = 8
const AUDIO_COLOR = 0x22c55e
const ACCENT_COLOR = 0x6c63ff
const SELECTION_BORDER = 2

function hexStringToNumber(hex: string): number {
  return parseInt(hex.replace('#', ''), 16)
}

export const AudioClip: React.FC<AudioClipProps> = React.memo(
  ({ clip, trackColorIndex, pixelsPerBeat, isSelected, trackRowY }) => {
    const clipWidth = Math.max(clip.lengthBeats * pixelsPerBeat, MIN_WIDTH)
    const clipX = clip.startBeat * pixelsPerBeat
    const trackColor = hexStringToNumber(TRACK_COLORS[trackColorIndex % TRACK_COLORS.length])
    const waveformHeight = CLIP_HEIGHT - TOP_BAR_HEIGHT - 4 // leave padding

    const drawBody = useCallback(
      (g: Graphics) => {
        g.clear()

        // Clip body — rounded rectangle with 30% opacity fill (audio green)
        g.roundRect(0, 0, clipWidth, CLIP_HEIGHT, BORDER_RADIUS)
        g.fill({ color: AUDIO_COLOR, alpha: 0.3 })

        // Top color bar — full opacity audio green
        g.roundRect(0, 0, clipWidth, TOP_BAR_HEIGHT, BORDER_RADIUS)
        g.fill({ color: AUDIO_COLOR, alpha: 1.0 })
        g.rect(0, TOP_BAR_HEIGHT / 2, clipWidth, TOP_BAR_HEIGHT / 2)
        g.fill({ color: AUDIO_COLOR, alpha: 1.0 })

        // Selection border
        if (isSelected) {
          g.roundRect(0, 0, clipWidth, CLIP_HEIGHT, BORDER_RADIUS)
          g.stroke({ color: ACCENT_COLOR, width: SELECTION_BORDER })
        }
      },
      [clipWidth, isSelected],
    )

    const drawWaveform = useCallback(
      (g: Graphics) => {
        g.clear()

        if (clip.peakData && clip.peakData.length > 0) {
          // Render waveform from peak data
          const vertices = peaksToVertices(clip.peakData, clipWidth, waveformHeight)
          if (vertices.length >= 4) {
            g.poly(vertices)
            g.fill({ color: trackColor, alpha: 0.5 })
          }
        } else {
          // No peak data — draw a dashed center line
          const centerY = waveformHeight / 2
          const dashLength = 4
          const gapLength = 3
          let x = 0
          while (x < clipWidth) {
            const end = Math.min(x + dashLength, clipWidth)
            g.moveTo(x, centerY)
            g.lineTo(end, centerY)
            x = end + gapLength
          }
          g.stroke({ color: trackColor, alpha: 0.4, width: 1 })
        }
      },
      [clip.peakData, clipWidth, waveformHeight, trackColor],
    )

    return (
      <pixiContainer
        x={clipX}
        y={trackRowY}
        eventMode="static"
        cursor="pointer"
        cullable={true}
      >
        <pixiGraphics draw={drawBody} />
        <pixiGraphics
          draw={drawWaveform}
          y={TOP_BAR_HEIGHT + 2}
        />
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

AudioClip.displayName = 'AudioClip'
