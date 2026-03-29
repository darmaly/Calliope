import { useRef, useCallback } from 'react'
import { panToString } from '../../utils/mixer-helpers'

interface PanKnobProps {
  value: number       // -1.0 to 1.0
  onChange: (value: number) => void
  size?: number       // diameter in px, default 32
  color?: string      // accent color
}

/**
 * Rotary pan knob.
 * Drag up/down or left/right to change pan.
 * Range: -1 (100L) to +1 (100R), center = 0.
 * Double-click resets to center.
 */
export function PanKnob({ value, onChange, size = 32, color = '#4a9eff' }: PanKnobProps) {
  const startYRef = useRef(0)
  const startValRef = useRef(0)
  const draggingRef = useRef(false)

  // Map -1..1 to angle: -135 to +135 degrees
  const angle = value * 135

  const handlePointerDown = useCallback(
    (e: React.PointerEvent) => {
      draggingRef.current = true
      startYRef.current = e.clientY
      startValRef.current = value
      ;(e.currentTarget as HTMLElement).setPointerCapture(e.pointerId)
    },
    [value],
  )

  const handlePointerMove = useCallback(
    (e: React.PointerEvent) => {
      if (!draggingRef.current) return
      const deltaY = startYRef.current - e.clientY
      const sensitivity = 0.01
      const newVal = Math.max(-1, Math.min(1, startValRef.current + deltaY * sensitivity))
      onChange(newVal)
    },
    [onChange],
  )

  const handlePointerUp = useCallback(() => {
    draggingRef.current = false
  }, [])

  const handleDoubleClick = useCallback(() => {
    onChange(0) // Reset to center
  }, [onChange])

  const r = size / 2
  const indicatorLength = r - 4

  return (
    <div
      className="flex flex-col items-center gap-0.5"
      style={{ width: size + 8 }}
    >
      <svg
        width={size}
        height={size}
        className="cursor-pointer"
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
        onDoubleClick={handleDoubleClick}
      >
        {/* Background circle */}
        <circle
          cx={r}
          cy={r}
          r={r - 2}
          fill="#2a2a4a"
          stroke="#3a3a5a"
          strokeWidth={1.5}
        />
        {/* Indicator line */}
        <line
          x1={r}
          y1={r}
          x2={r + indicatorLength * Math.sin((angle * Math.PI) / 180)}
          y2={r - indicatorLength * Math.cos((angle * Math.PI) / 180)}
          stroke={color}
          strokeWidth={2}
          strokeLinecap="round"
        />
        {/* Center dot */}
        <circle cx={r} cy={r} r={2} fill="#999999" />
      </svg>
      {/* Pan readout */}
      <span className="text-[9px] text-[#999999] select-none">
        {panToString(value)}
      </span>
    </div>
  )
}
