import React, { useCallback } from 'react'
import type { Graphics } from 'pixi.js'
import type { MidiNote } from '../../types/piano-roll'
import { velocityToAlpha } from '../../utils/piano-helpers'

interface NoteRectProps {
  note: MidiNote
  pixelsPerBeat: number
  noteRowHeight: number
  selected: boolean
  trackColorHex: string
}

const ACCENT_COLOR = 0x6c63ff
const SELECTION_BORDER = 2
const BORDER_RADIUS = 2

function hexStringToNumber(hex: string): number {
  return parseInt(hex.replace('#', ''), 16)
}

export const NoteRect: React.FC<NoteRectProps> = React.memo(
  ({ note, pixelsPerBeat, noteRowHeight, selected, trackColorHex }) => {
    const x = note.startBeat * pixelsPerBeat
    const y = (127 - note.pitch) * noteRowHeight
    const width = note.lengthBeats * pixelsPerBeat
    const height = noteRowHeight - 1 // 1px gap between rows
    const alpha = velocityToAlpha(note.velocity)
    const colorNum = hexStringToNumber(trackColorHex)

    const draw = useCallback(
      (g: Graphics) => {
        g.clear()

        // Note body
        g.roundRect(0, 0, Math.max(width, 2), height, BORDER_RADIUS)
        g.fill({ color: colorNum, alpha })

        // Selection border
        if (selected) {
          g.roundRect(0, 0, Math.max(width, 2), height, BORDER_RADIUS)
          g.stroke({ width: SELECTION_BORDER, color: ACCENT_COLOR, alpha: 1 })
        }
      },
      [width, height, colorNum, alpha, selected],
    )

    return (
      <pixiContainer x={x} y={y} cullable={true}>
        <pixiGraphics draw={draw} />
      </pixiContainer>
    )
  },
)

NoteRect.displayName = 'NoteRect'
