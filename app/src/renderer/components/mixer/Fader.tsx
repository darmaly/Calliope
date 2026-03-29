import { useCallback, useRef } from 'react'
import { faderToGain, gainToFader, formatDb } from '../../utils/mixer-helpers'

interface FaderProps {
  value: number // linear gain (0 to ~2.0)
  onChange: (gain: number) => void
}

const TRACK_HEIGHT = 160
const TRACK_WIDTH = 4
const THUMB_WIDTH = 12
const THUMB_HEIGHT = 20
const CONTAINER_WIDTH = 20

const DB_MARKS = [
  { db: 6, label: '+6' },
  { db: 0, label: '0' },
  { db: -6, label: '-6' },
  { db: -12, label: '-12' },
  { db: -24, label: '-24' },
  { db: -48, label: '-48' },
]

function dbToPosition(db: number): number {
  const minDb = -60
  const maxDb = 6
  return (db - minDb) / (maxDb - minDb)
}

export function Fader({ value, onChange }: FaderProps) {
  const trackRef = useRef<HTMLDivElement>(null)
  const dragging = useRef(false)

  const faderPos = gainToFader(value)

  const positionFromClientY = useCallback((clientY: number): number => {
    if (!trackRef.current) return 0
    const rect = trackRef.current.getBoundingClientRect()
    // Invert: top of track = position 1, bottom = position 0
    const raw = 1 - (clientY - rect.top) / rect.height
    return Math.max(0, Math.min(1, raw))
  }, [])

  const handlePointerDown = useCallback(
    (e: React.PointerEvent) => {
      e.preventDefault()
      ;(e.target as HTMLElement).setPointerCapture(e.pointerId)
      dragging.current = true
    },
    [],
  )

  const handlePointerMove = useCallback(
    (e: React.PointerEvent) => {
      if (!dragging.current) return
      const pos = positionFromClientY(e.clientY)
      onChange(faderToGain(pos))
    },
    [onChange, positionFromClientY],
  )

  const handlePointerUp = useCallback(
    (e: React.PointerEvent) => {
      if (!dragging.current) return
      ;(e.target as HTMLElement).releasePointerCapture(e.pointerId)
      dragging.current = false
    },
    [],
  )

  const handleTrackClick = useCallback(
    (e: React.PointerEvent) => {
      const pos = positionFromClientY(e.clientY)
      onChange(faderToGain(pos))
    },
    [onChange, positionFromClientY],
  )

  const handleDoubleClick = useCallback(() => {
    onChange(1.0) // Reset to 0dB (unity)
  }, [onChange])

  // Thumb position: bottom-up, offset by half thumb height
  const thumbBottom = faderPos * TRACK_HEIGHT - THUMB_HEIGHT / 2

  return (
    <div style={{ width: CONTAINER_WIDTH + 24, height: TRACK_HEIGHT + 20 }}>
      <div style={{ display: 'flex', height: TRACK_HEIGHT }}>
        {/* dB scale markings */}
        <div
          style={{
            width: 24,
            height: TRACK_HEIGHT,
            position: 'relative',
            flexShrink: 0,
          }}
        >
          {DB_MARKS.map(({ db, label }) => {
            const pos = dbToPosition(db)
            return (
              <span
                key={db}
                style={{
                  position: 'absolute',
                  right: 2,
                  bottom: pos * TRACK_HEIGHT - 5,
                  fontSize: 10,
                  color: '#999999',
                  lineHeight: 1,
                  userSelect: 'none',
                }}
              >
                {label}
              </span>
            )
          })}
        </div>

        {/* Track + thumb */}
        <div
          ref={trackRef}
          style={{
            width: CONTAINER_WIDTH,
            height: TRACK_HEIGHT,
            position: 'relative',
            cursor: 'pointer',
          }}
          onPointerDown={handleTrackClick}
        >
          {/* Track background */}
          <div
            style={{
              position: 'absolute',
              left: (CONTAINER_WIDTH - TRACK_WIDTH) / 2,
              top: 0,
              width: TRACK_WIDTH,
              height: TRACK_HEIGHT,
              backgroundColor: '#3a3a5a',
              borderRadius: 2,
            }}
          />

          {/* Thumb */}
          <div
            style={{
              position: 'absolute',
              left: (CONTAINER_WIDTH - THUMB_WIDTH) / 2,
              bottom: Math.max(0, Math.min(TRACK_HEIGHT - THUMB_HEIGHT, thumbBottom)),
              width: THUMB_WIDTH,
              height: THUMB_HEIGHT,
              backgroundColor: '#eeeeee',
              borderRadius: 2,
              cursor: 'grab',
              zIndex: 1,
            }}
            onPointerDown={(e) => {
              e.stopPropagation()
              handlePointerDown(e)
            }}
            onPointerMove={handlePointerMove}
            onPointerUp={handlePointerUp}
            onDoubleClick={handleDoubleClick}
          />
        </div>
      </div>

      {/* dB readout */}
      <div
        style={{
          textAlign: 'center',
          fontSize: 9,
          color: '#999999',
          marginTop: 2,
          userSelect: 'none',
          paddingLeft: 24,
        }}
      >
        {formatDb(value)}
      </div>
    </div>
  )
}
