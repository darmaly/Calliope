import { useCallback, useMemo } from 'react'
import type { Graphics, TextStyle as PixiTextStyle } from 'pixi.js'
import { pitchToNoteName, isBlackKey, pitchToY } from '../../utils/piano-helpers'

interface PianoKeyboardProps {
  viewportHeight: number
  scrollY: number
  noteRowHeight: number
}

const KEYBOARD_WIDTH = 60
const WHITE_KEY_COLOR = 0x2a2a4a
const BLACK_KEY_COLOR = 0x1a1a2e
const BORDER_COLOR = 0x3a3a5a

export function PianoKeyboard({ viewportHeight, scrollY, noteRowHeight }: PianoKeyboardProps) {
  // Calculate visible pitch range
  const { startPitch, endPitch } = useMemo(() => {
    const topPitch = 127 - Math.floor(scrollY / noteRowHeight)
    const bottomPitch = 127 - Math.ceil((scrollY + viewportHeight) / noteRowHeight)
    return {
      startPitch: Math.max(0, bottomPitch - 1),
      endPitch: Math.min(127, topPitch + 1),
    }
  }, [scrollY, viewportHeight, noteRowHeight])

  const showLabels = noteRowHeight >= 10

  const draw = useCallback(
    (g: Graphics) => {
      g.clear()
      if (viewportHeight <= 0) return

      for (let pitch = startPitch; pitch <= endPitch; pitch++) {
        const y = pitchToY(pitch, noteRowHeight)
        const black = isBlackKey(pitch)

        // Key background
        g.rect(0, y, KEYBOARD_WIDTH, noteRowHeight)
        g.fill({ color: black ? BLACK_KEY_COLOR : WHITE_KEY_COLOR })

        // Bottom border
        g.moveTo(0, y + noteRowHeight)
        g.lineTo(KEYBOARD_WIDTH, y + noteRowHeight)
        g.stroke({ width: 1, color: BORDER_COLOR, alpha: 0.6 })
      }

      // Right border
      g.moveTo(KEYBOARD_WIDTH, scrollY)
      g.lineTo(KEYBOARD_WIDTH, scrollY + viewportHeight)
      g.stroke({ width: 1, color: BORDER_COLOR, alpha: 1 })
    },
    [startPitch, endPitch, noteRowHeight, scrollY, viewportHeight],
  )

  // Text labels rendered separately since PixiJS Graphics doesn't do text
  const labels = useMemo(() => {
    if (!showLabels) return []
    const result: { pitch: number; text: string; y: number; isC: boolean }[] = []
    for (let pitch = startPitch; pitch <= endPitch; pitch++) {
      if (isBlackKey(pitch)) continue
      const isC = pitch % 12 === 0
      // Only show label on C notes (every octave) or if row height is large enough for all white keys
      if (isC || noteRowHeight >= 18) {
        result.push({
          pitch,
          text: pitchToNoteName(pitch),
          y: pitchToY(pitch, noteRowHeight),
          isC,
        })
      }
    }
    return result
  }, [startPitch, endPitch, noteRowHeight, showLabels])

  const labelStyle = useMemo(
    (): Partial<PixiTextStyle> => ({
      fontFamily: 'Inter, sans-serif',
      fontSize: 10,
      fill: 0xcccccc,
    }),
    [],
  )

  const cLabelStyle = useMemo(
    (): Partial<PixiTextStyle> => ({
      fontFamily: 'Inter, sans-serif',
      fontSize: 10,
      fontWeight: '500',
      fill: 0xeeeeee,
    }),
    [],
  )

  return (
    <pixiContainer>
      <pixiGraphics draw={draw} />
      {labels.map((l) => (
        <pixiText
          key={l.pitch}
          text={l.text}
          x={4}
          y={l.y + noteRowHeight / 2 - 5}
          style={l.isC ? cLabelStyle : labelStyle}
        />
      ))}
    </pixiContainer>
  )
}

export { KEYBOARD_WIDTH }
