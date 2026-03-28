import { useRef } from 'react'
import { usePianoRollStore } from '../../stores/piano-roll-store'
import { useTimelineStore } from '../../stores/timeline-store'
import { useShallow } from 'zustand/shallow'
import { TRACK_COLORS } from '../../utils/colors'
import { PianoRollToolbar } from './PianoRollToolbar'
import { PianoRollCanvas } from './PianoRollCanvas'

export function PianoRollPanel() {
  const activeClipId = usePianoRollStore((s) => s.activeClipId)
  const canvasContainerRef = useRef<HTMLDivElement>(null)

  const { clips, tracks } = useTimelineStore(
    useShallow((s) => ({
      clips: s.clips,
      tracks: s.tracks,
    })),
  )

  if (!activeClipId) {
    return (
      <div className="h-full flex flex-col items-center justify-center bg-[#1a1a2e]">
        <p className="text-[13px] font-medium text-[#eeeeee]">No MIDI clip selected</p>
        <p className="text-[13px] text-[#999999] mt-1">
          Double-click a MIDI clip in the timeline to open it here.
        </p>
      </div>
    )
  }

  // Determine track color for the active clip
  const clip = clips[activeClipId]
  const track = clip ? tracks.find((t) => t.id === clip.trackId) : undefined
  const colorIndex = track?.colorIndex ?? 0
  const trackColorHex = TRACK_COLORS[colorIndex % TRACK_COLORS.length]

  return (
    <div className="h-full flex flex-col bg-[#1a1a2e]">
      <PianoRollToolbar />
      <div ref={canvasContainerRef} className="flex-1 min-h-0 relative">
        <PianoRollCanvas containerRef={canvasContainerRef} trackColorHex={trackColorHex} />
      </div>
    </div>
  )
}
