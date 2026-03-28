import { useTimelineStore } from '../../stores/timeline-store'
import { usePianoRollStore } from '../../stores/piano-roll-store'
import { useShallow } from 'zustand/shallow'
import { beatToPixel } from '../../utils/beat-math'

const KEYBOARD_WIDTH = 60

export function PianoRollPlayhead() {
  const { currentBeat, pixelsPerBeat } = useTimelineStore(
    useShallow((s) => ({
      currentBeat: s.currentBeat,
      pixelsPerBeat: s.pixelsPerBeat,
    })),
  )
  const scrollX = usePianoRollStore((s) => s.scrollX)

  const xPos = beatToPixel(currentBeat, pixelsPerBeat, scrollX)

  // Only render when visible in the canvas area
  if (xPos < 0 || xPos > 10000) return null

  return (
    <div
      className="absolute top-0 bottom-0 w-px pointer-events-none"
      style={{
        left: xPos + KEYBOARD_WIDTH,
        backgroundColor: '#6c63ff',
        zIndex: 15,
      }}
    />
  )
}
