import { useCallback, useRef } from 'react'
import { panToString } from '../../utils/mixer-helpers'

interface PanKnobProps {
  value: number // -1.0 to +1.0
  onChange: (pan: number) => void
}

const KNOB_SIZE = 32
const INDICATOR_LENGTH = 12

export function PanKnob({ value, onChange }: PanKnobProps) {
  const startYRef = useRef(0)
  const startValueRef = useRef(0)
  const dragging = useRef(false)

  const handlePointerDown = useCallback(
    (e: React.PointerEvent) => {
      e.preventDefault()
      ;(e.target as HTMLElement).setPointerCapture(e.pointerId)
      startYRef.current = e.clientY
      startValueRef.current = value
      dragging.current = true
    },
    [value],
  )

  const handlePointerMove = useCallback(
    (e: React.PointerEvent) => {
      if (!dragging.current) return
      const delta = (startYRef.current - e.clientY) / 100
      const newValue = Math.max(-1, Math.min(1, startValueRef.current + delta))
      onChange(newValue)
    },
    [onChange],
  )

  const handlePointerUp = useCallback(
    (e: React.PointerEvent) => {
      if (!dragging.current) return
      ;(e.target as HTMLElement).releasePointerCapture(e.pointerId)
      dragging.current = false
    },
    [],
  )

  const handleDoubleClick = useCallback(() => {
    onChange(0)
  }, [onChange])

  // Rotation: value * 135 degrees (range -135 to +135)
  const rotation = value * 135

  return (
    <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 2 }}>
      <div
        style={{
          width: KNOB_SIZE,
          height: KNOB_SIZE,
          borderRadius: '50%',
          backgroundColor: '#3a3a5a',
          position: 'relative',
          cursor: 'pointer',
          touchAction: 'none',
        }}
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
        onDoubleClick={handleDoubleClick}
      >
        {/* Indicator line from center to edge */}
        <div
          style={{
            position: 'absolute',
            left: KNOB_SIZE / 2 - 1,
            top: KNOB_SIZE / 2 - INDICATOR_LENGTH,
            width: 2,
            height: INDICATOR_LENGTH,
            backgroundColor: '#eeeeee',
            borderRadius: 1,
            transformOrigin: `1px ${INDICATOR_LENGTH}px`,
            transform: `rotate(${rotation}deg)`,
            pointerEvents: 'none',
          }}
        />
      </div>
      <span style={{ fontSize: 10, color: '#999999', userSelect: 'none' }}>
        {panToString(value)}
      </span>
    </div>
  )
}
