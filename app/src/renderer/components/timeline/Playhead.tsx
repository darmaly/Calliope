import { useTimelineStore } from '../../stores/timeline-store'
import { useShallow } from 'zustand/shallow'
import { beatToPixel } from '../../utils/beat-math'

const HEADER_WIDTH = 200

export function Playhead() {
  const { currentBeat, pixelsPerBeat, scrollX } = useTimelineStore(
    useShallow((s) => ({
      currentBeat: s.currentBeat,
      pixelsPerBeat: s.pixelsPerBeat,
      scrollX: s.scrollX,
    }))
  )

  const xPos = beatToPixel(currentBeat, pixelsPerBeat, scrollX)

  // Only render when visible in the canvas area
  if (xPos < 0 || xPos > 10000) return null

  return (
    <div
      className="absolute top-0 bottom-0 w-px bg-[#6c63ff] pointer-events-none z-10"
      style={{ left: xPos + HEADER_WIDTH }}
    />
  )
}
