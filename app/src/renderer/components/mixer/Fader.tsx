import { useRef, useCallback } from 'react'
import { formatDb } from '../../utils/mixer-helpers'

interface FaderProps {
  value: number       // linear 0..~1.995
  onChange: (value: number) => void
  height?: number     // px, default 160
  color?: string      // track color accent
}

/**
 * Vertical fader control for volume.
 * Maps linear gain (0 to ~1.995) to vertical slider position.
 * Displays dB value below.
 */
export function Fader({ value, onChange, height = 160, color = '#4a9eff' }: FaderProps) {
  const trackRef = useRef<HTMLDivElement>(null)
  const draggingRef = useRef(false)

  // Map linear 0..~1.995 to 0..1 position (0 = bottom, 1 = top)
  // Use a ~2.0 max to cover +6dB
  const maxLinear = 1.9953
  const position = Math.min(1, value / maxLinear)

  const updateFromPointer = useCallback(
    (clientY: number) => {
      const track = trackRef.current
      if (!track) return
      const rect = track.getBoundingClientRect()
      const relY = rect.bottom - clientY
      const normalized = Math.max(0, Math.min(1, relY / rect.height))
      onChange(normalized * maxLinear)
    },
    [onChange],
  )

  const handlePointerDown = useCallback(
    (e: React.PointerEvent) => {
      draggingRef.current = true
      ;(e.currentTarget as HTMLElement).setPointerCapture(e.pointerId)
      updateFromPointer(e.clientY)
    },
    [updateFromPointer],
  )

  const handlePointerMove = useCallback(
    (e: React.PointerEvent) => {
      if (!draggingRef.current) return
      updateFromPointer(e.clientY)
    },
    [updateFromPointer],
  )

  const handlePointerUp = useCallback(() => {
    draggingRef.current = false
  }, [])

  const handleDoubleClick = useCallback(() => {
    onChange(1.0) // Reset to 0 dB
  }, [onChange])

  const thumbBottom = position * (height - 12) // 12px thumb height

  return (
    <div className="flex flex-col items-center gap-1">
      {/* Fader track */}
      <div
        ref={trackRef}
        className="relative cursor-pointer"
        style={{ width: 24, height }}
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
        onDoubleClick={handleDoubleClick}
      >
        {/* Track groove */}
        <div
          className="absolute left-1/2 -translate-x-1/2 w-[4px] rounded-full bg-[#3a3a5a]"
          style={{ top: 0, bottom: 0 }}
        />
        {/* Fill from bottom */}
        <div
          className="absolute left-1/2 -translate-x-1/2 w-[4px] rounded-full"
          style={{
            bottom: 0,
            height: `${position * 100}%`,
            backgroundColor: color,
            opacity: 0.6,
          }}
        />
        {/* Thumb */}
        <div
          className="absolute left-0 w-full h-[12px] rounded-sm"
          style={{
            bottom: thumbBottom,
            backgroundColor: '#cccccc',
            boxShadow: '0 1px 3px rgba(0,0,0,0.4)',
          }}
        />
        {/* 0dB mark */}
        <div
          className="absolute left-0 w-full h-[1px] bg-[#666666]"
          style={{ bottom: `${(1.0 / maxLinear) * 100}%` }}
        />
      </div>
      {/* dB readout */}
      <span className="text-[10px] text-[#999999] select-none whitespace-nowrap">
        {formatDb(value)}
      </span>
    </div>
  )
}
