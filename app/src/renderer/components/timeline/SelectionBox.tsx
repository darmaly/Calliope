import React, { useCallback } from 'react'
import type { Graphics } from 'pixi.js'

export interface SelectionBoxProps {
  startX: number
  startY: number
  endX: number
  endY: number
  visible: boolean
}

const ACCENT_COLOR = 0x6c63ff

export const SelectionBox: React.FC<SelectionBoxProps> = React.memo(
  ({ startX, startY, endX, endY, visible }) => {
    const draw = useCallback(
      (g: Graphics) => {
        g.clear()
        if (!visible) return

        const x = Math.min(startX, endX)
        const y = Math.min(startY, endY)
        const w = Math.abs(endX - startX)
        const h = Math.abs(endY - startY)

        // Fill with 10% opacity
        g.rect(x, y, w, h)
        g.fill({ color: ACCENT_COLOR, alpha: 0.1 })

        // Dashed border at 100% opacity
        const dashLen = 4
        const gapLen = 3

        // Helper to draw a dashed line
        const dashLine = (x1: number, y1: number, x2: number, y2: number) => {
          const dx = x2 - x1
          const dy = y2 - y1
          const dist = Math.sqrt(dx * dx + dy * dy)
          const ux = dx / dist
          const uy = dy / dist
          let pos = 0
          while (pos < dist) {
            const end = Math.min(pos + dashLen, dist)
            g.moveTo(x1 + ux * pos, y1 + uy * pos)
            g.lineTo(x1 + ux * end, y1 + uy * end)
            pos = end + gapLen
          }
        }

        dashLine(x, y, x + w, y) // top
        dashLine(x + w, y, x + w, y + h) // right
        dashLine(x + w, y + h, x, y + h) // bottom
        dashLine(x, y + h, x, y) // left
        g.stroke({ color: ACCENT_COLOR, alpha: 1.0, width: 1 })
      },
      [startX, startY, endX, endY, visible],
    )

    if (!visible) return null

    return <pixiGraphics draw={draw} />
  },
)

SelectionBox.displayName = 'SelectionBox'
