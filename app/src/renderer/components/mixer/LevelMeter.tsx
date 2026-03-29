import { useEffect, useRef } from 'react'

interface LevelMeterProps {
  leftLevel: number  // 0-1
  rightLevel: number // 0-1
  leftPeak: number   // 0-1
  rightPeak: number  // 0-1
}

const METER_WIDTH = 16  // 7px L + 2px gap + 7px R
const METER_HEIGHT = 160
const CHANNEL_WIDTH = 7
const GAP = 2

export function LevelMeter({ leftLevel, rightLevel, leftPeak, rightPeak }: LevelMeterProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null)

  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas) return
    const ctx = canvas.getContext('2d')
    if (!ctx) return

    // Clear
    ctx.fillStyle = '#1a1a2e'
    ctx.fillRect(0, 0, METER_WIDTH, METER_HEIGHT)

    // Create gradient (bottom to top: green -> amber -> red)
    const gradient = ctx.createLinearGradient(0, METER_HEIGHT, 0, 0)
    gradient.addColorStop(0.0, '#22c55e')   // green at bottom
    gradient.addColorStop(0.6, '#22c55e')   // green up to 60%
    gradient.addColorStop(0.9, '#f59e0b')   // amber at 90%
    gradient.addColorStop(1.0, '#ef4444')   // red at top

    // Draw left channel
    const leftH = leftLevel * METER_HEIGHT
    if (leftH > 0) {
      ctx.fillStyle = gradient
      ctx.fillRect(0, METER_HEIGHT - leftH, CHANNEL_WIDTH, leftH)
    }

    // Draw right channel
    const rightH = rightLevel * METER_HEIGHT
    if (rightH > 0) {
      ctx.fillStyle = gradient
      ctx.fillRect(CHANNEL_WIDTH + GAP, METER_HEIGHT - rightH, CHANNEL_WIDTH, rightH)
    }

    // Peak hold indicators
    ctx.fillStyle = '#eeeeee'
    if (leftPeak > 0) {
      const peakY = METER_HEIGHT - leftPeak * METER_HEIGHT
      ctx.fillRect(0, peakY, CHANNEL_WIDTH, 1)
    }
    if (rightPeak > 0) {
      const peakY = METER_HEIGHT - rightPeak * METER_HEIGHT
      ctx.fillRect(CHANNEL_WIDTH + GAP, peakY, CHANNEL_WIDTH, 1)
    }
  }, [leftLevel, rightLevel, leftPeak, rightPeak])

  return (
    <canvas
      ref={canvasRef}
      width={METER_WIDTH}
      height={METER_HEIGHT}
      style={{
        width: METER_WIDTH,
        height: METER_HEIGHT,
        borderRadius: 2,
        border: '1px solid #3a3a5a',
      }}
    />
  )
}
