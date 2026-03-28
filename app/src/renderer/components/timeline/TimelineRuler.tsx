import { useRef, useEffect, useCallback } from 'react'
import { useTimelineStore } from '../../stores/timeline-store'
import { useShallow } from 'zustand/shallow'

export function TimelineRuler() {
  const { pixelsPerBeat, scrollX, timeSignature } = useTimelineStore(
    useShallow((s) => ({
      pixelsPerBeat: s.pixelsPerBeat,
      scrollX: s.scrollX,
      timeSignature: s.timeSignature,
    }))
  )

  const canvasRef = useRef<HTMLCanvasElement>(null)
  const containerRef = useRef<HTMLDivElement>(null)

  const draw = useCallback(() => {
    const canvas = canvasRef.current
    const container = containerRef.current
    if (!canvas || !container) return

    const ctx = canvas.getContext('2d')
    if (!ctx) return

    const width = container.clientWidth
    const height = 48
    const dpr = window.devicePixelRatio || 1

    canvas.width = width * dpr
    canvas.height = height * dpr
    canvas.style.width = `${width}px`
    canvas.style.height = `${height}px`
    ctx.scale(dpr, dpr)

    // Clear
    ctx.clearRect(0, 0, width, height)

    const beatsPerBar = timeSignature.numerator
    const pixelsPerBar = pixelsPerBeat * beatsPerBar

    // Calculate visible range
    const startBeat = scrollX / pixelsPerBeat
    const endBeat = startBeat + width / pixelsPerBeat
    const startBar = Math.floor(startBeat / beatsPerBar)
    const endBar = Math.ceil(endBeat / beatsPerBar) + 1

    // Draw bar/beat markers
    for (let bar = startBar; bar <= endBar; bar++) {
      const barBeat = bar * beatsPerBar
      const x = barBeat * pixelsPerBeat - scrollX

      if (x < -50 || x > width + 50) continue

      // Bar line
      ctx.strokeStyle = 'rgba(255, 255, 255, 0.2)'
      ctx.lineWidth = 1
      ctx.beginPath()
      ctx.moveTo(x, 30)
      ctx.lineTo(x, height)
      ctx.stroke()

      // Bar number label (1-indexed)
      ctx.fillStyle = '#999999'
      ctx.font = '11px system-ui, sans-serif'
      ctx.fillText(`${bar + 1}`, x + 4, 24)

      // Beat subdivisions
      if (pixelsPerBar > 40) {
        for (let beat = 1; beat < beatsPerBar; beat++) {
          const beatX = (barBeat + beat) * pixelsPerBeat - scrollX
          if (beatX < 0 || beatX > width) continue

          ctx.strokeStyle = 'rgba(255, 255, 255, 0.08)'
          ctx.lineWidth = 1
          ctx.beginPath()
          ctx.moveTo(beatX, 36)
          ctx.lineTo(beatX, height)
          ctx.stroke()
        }
      }
    }
  }, [pixelsPerBeat, scrollX, timeSignature])

  useEffect(() => {
    draw()
  }, [draw])

  useEffect(() => {
    const container = containerRef.current
    if (!container) return

    const observer = new ResizeObserver(() => draw())
    observer.observe(container)
    return () => observer.disconnect()
  }, [draw])

  return (
    <div
      ref={containerRef}
      className="h-[48px] bg-[#252542] border-b border-[#3a3a5a] relative shrink-0"
    >
      <canvas ref={canvasRef} className="absolute inset-0" />
    </div>
  )
}
