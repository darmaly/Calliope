import { useRef, useEffect, useCallback, useState } from 'react'
import { useTimelineStore } from '../../stores/timeline-store'
import { useShallow } from 'zustand/shallow'
import { pixelToBeat, snapToBeat, beatToPixel } from '../../utils/beat-math'

export function TimelineRuler() {
  const { pixelsPerBeat, scrollX, timeSignature, loopRegion } = useTimelineStore(
    useShallow((s) => ({
      pixelsPerBeat: s.pixelsPerBeat,
      scrollX: s.scrollX,
      timeSignature: s.timeSignature,
      loopRegion: s.loopRegion,
    })),
  )

  const setLoopRegion = useTimelineStore((s) => s.setLoopRegion)

  const canvasRef = useRef<HTMLCanvasElement>(null)
  const containerRef = useRef<HTMLDivElement>(null)

  // Drag state for loop region creation
  const [isDragging, setIsDragging] = useState(false)
  const [dragStart, setDragStart] = useState(0)
  const [dragEnd, setDragEnd] = useState(0)

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

    // Draw loop region on ruler
    const lr = isDragging
      ? { startBeat: Math.min(dragStart, dragEnd), endBeat: Math.max(dragStart, dragEnd) }
      : loopRegion
    if (lr) {
      const loopStartX = beatToPixel(lr.startBeat, pixelsPerBeat, scrollX)
      const loopEndX = beatToPixel(lr.endBeat, pixelsPerBeat, scrollX)
      ctx.fillStyle = 'rgba(108, 99, 255, 0.25)'
      ctx.fillRect(loopStartX, 0, loopEndX - loopStartX, height)
    }

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
  }, [pixelsPerBeat, scrollX, timeSignature, loopRegion, isDragging, dragStart, dragEnd])

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

  const handlePointerDown = useCallback(
    (e: React.PointerEvent<HTMLDivElement>) => {
      const rect = (e.currentTarget as HTMLElement).getBoundingClientRect()
      const px = e.clientX - rect.left
      const state = useTimelineStore.getState()
      const beat = pixelToBeat(px, state.pixelsPerBeat, state.scrollX)
      const snapped = state.snapEnabled ? snapToBeat(beat, state.gridResolution) : beat

      if (e.shiftKey) {
        // Shift+click+drag: create loop region
        setIsDragging(true)
        setDragStart(snapped)
        setDragEnd(snapped)
        ;(e.currentTarget as HTMLElement).setPointerCapture(e.pointerId)
      }
      // Plain click on ruler could move playhead (future feature — no-op for now)
    },
    [],
  )

  const handlePointerMove = useCallback(
    (e: React.PointerEvent<HTMLDivElement>) => {
      if (!isDragging) return
      const rect = (e.currentTarget as HTMLElement).getBoundingClientRect()
      const px = e.clientX - rect.left
      const state = useTimelineStore.getState()
      const beat = pixelToBeat(px, state.pixelsPerBeat, state.scrollX)
      const snapped = state.snapEnabled ? snapToBeat(beat, state.gridResolution) : beat
      setDragEnd(snapped)
    },
    [isDragging],
  )

  const handlePointerUp = useCallback(
    (e: React.PointerEvent<HTMLDivElement>) => {
      if (!isDragging) return
      setIsDragging(false)

      const startBeat = Math.min(dragStart, dragEnd)
      const endBeat = Math.max(dragStart, dragEnd)

      if (endBeat - startBeat > 0.01) {
        setLoopRegion({ startBeat, endBeat })
        try {
          window.calliope?.setLoopRegion(startBeat, endBeat, true)
        } catch {
          // Engine not available
        }
      }

      ;(e.currentTarget as HTMLElement).releasePointerCapture(e.pointerId)
    },
    [isDragging, dragStart, dragEnd, setLoopRegion],
  )

  const handleDoubleClick = useCallback(() => {
    // Double-click on ruler with existing loop region: remove it
    if (loopRegion) {
      setLoopRegion(null)
      try {
        window.calliope?.setLoopRegion(0, 0, false)
      } catch {
        // Engine not available
      }
    }
  }, [loopRegion, setLoopRegion])

  return (
    <div
      ref={containerRef}
      className="h-[48px] bg-[#252542] border-b border-[#3a3a5a] relative shrink-0"
      onPointerDown={handlePointerDown}
      onPointerMove={handlePointerMove}
      onPointerUp={handlePointerUp}
      onDoubleClick={handleDoubleClick}
    >
      <canvas ref={canvasRef} className="absolute inset-0 pointer-events-none" />
    </div>
  )
}
