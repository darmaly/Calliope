import { useRef, useEffect } from 'react'

interface LevelMeterProps {
  peakL: number   // 0..1 linear level
  peakR: number   // 0..1 linear level
  height?: number // px, default 160
  width?: number  // px, default 12
}

/**
 * Stereo level meter rendered on a Canvas 2D element.
 * Green (low) -> Yellow (mid) -> Red (high) gradient.
 * Refreshes via requestAnimationFrame when levels change.
 */
export function LevelMeter({ peakL, peakR, height = 160, width = 12 }: LevelMeterProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null)
  const prevLevelsRef = useRef({ peakL: 0, peakR: 0 })

  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas) return
    const ctx = canvas.getContext('2d')
    if (!ctx) return

    // Apply decay for smooth falloff
    const decay = 0.92
    const prevL = prevLevelsRef.current.peakL
    const prevR = prevLevelsRef.current.peakR
    const displayL = Math.max(peakL, prevL * decay)
    const displayR = Math.max(peakR, prevR * decay)
    prevLevelsRef.current = { peakL: displayL, peakR: displayR }

    const barWidth = (width - 2) / 2 // 2 bars with 2px gap
    const dpr = window.devicePixelRatio || 1

    canvas.width = width * dpr
    canvas.height = height * dpr
    ctx.scale(dpr, dpr)

    // Clear
    ctx.fillStyle = '#1a1a2e'
    ctx.fillRect(0, 0, width, height)

    // Draw each channel
    const drawBar = (level: number, x: number) => {
      const barHeight = level * height
      if (barHeight <= 0) return

      const y = height - barHeight

      // Create gradient: green at bottom, yellow at -12dB, red at 0dB
      const grad = ctx.createLinearGradient(0, height, 0, 0)
      grad.addColorStop(0, '#22c55e')     // green
      grad.addColorStop(0.6, '#22c55e')   // green until -12dB region
      grad.addColorStop(0.8, '#f59e0b')   // yellow
      grad.addColorStop(0.95, '#ef4444')  // red
      grad.addColorStop(1.0, '#ef4444')   // red at top

      ctx.fillStyle = grad
      ctx.fillRect(x, y, barWidth, barHeight)
    }

    drawBar(displayL, 0)
    drawBar(displayR, barWidth + 2)
  }, [peakL, peakR, height, width])

  return (
    <canvas
      ref={canvasRef}
      style={{ width, height }}
      className="rounded-sm"
    />
  )
}
