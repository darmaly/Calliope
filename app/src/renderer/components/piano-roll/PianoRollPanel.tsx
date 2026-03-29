import { useRef } from 'react'
import { usePianoRollStore } from '../../stores/piano-roll-store'
import { useTimelineStore } from '../../stores/timeline-store'
import { useShallow } from 'zustand/shallow'
import { TRACK_COLORS } from '../../utils/colors'
import { PianoRollToolbar } from './PianoRollToolbar'
import { PianoRollCanvas } from './PianoRollCanvas'
import { VelocityLaneDom } from './VelocityLaneDom'
import { usePianoRollShortcuts } from '../../hooks/use-piano-roll-shortcuts'

export function PianoRollPanel() {
  const activeClipId = usePianoRollStore((s) => s.activeClipId)
  const velocityLaneVisible = usePianoRollStore((s) => s.velocityLaneVisible)
  const velocityLaneHeight = usePianoRollStore((s) => s.velocityLaneHeight)
  const canvasContainerRef = useRef<HTMLDivElement>(null)

  // Activate piano roll keyboard shortcuts when panel is mounted
  usePianoRollShortcuts()

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
      {/* Velocity lane with resizable divider */}
      {velocityLaneVisible && (
        <>
          <div
            className="h-[8px] cursor-row-resize bg-[#252542] hover:bg-[#3a3a5a] flex items-center justify-center shrink-0"
            onPointerDown={(e) => {
              const startY = e.clientY
              const startH = velocityLaneHeight
              const el = e.currentTarget
              el.setPointerCapture(e.pointerId)

              const onMove = (ev: PointerEvent) => {
                const delta = startY - ev.clientY
                const newH = Math.max(60, Math.min(300, startH + delta))
                usePianoRollStore.getState().setVelocityLaneHeight(newH)
              }
              const onUp = () => {
                el.removeEventListener('pointermove', onMove)
                el.removeEventListener('pointerup', onUp)
              }
              el.addEventListener('pointermove', onMove)
              el.addEventListener('pointerup', onUp)
            }}
          >
            <div className="w-8 h-[2px] bg-[#3a3a5a] rounded" />
          </div>
          <VelocityLaneDom
            height={velocityLaneHeight}
            trackColorHex={trackColorHex}
          />
        </>
      )}
    </div>
  )
}
