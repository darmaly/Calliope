import { Grid3x3 } from 'lucide-react'
import { usePianoRollStore } from '../../stores/piano-roll-store'
import { useTimelineStore } from '../../stores/timeline-store'
import { useShallow } from 'zustand/shallow'
import { GridResolutionSelect } from '../shared/GridResolutionSelect'
import { quantizeSelectedNotes } from '../../utils/note-operations'

export function PianoRollToolbar() {
  const { activeClipId, velocityLaneVisible } = usePianoRollStore(
    useShallow((s) => ({
      activeClipId: s.activeClipId,
      velocityLaneVisible: s.velocityLaneVisible,
    })),
  )

  const clips = useTimelineStore((s) => s.clips)
  const clipName = activeClipId ? clips[activeClipId]?.name ?? 'Untitled Clip' : ''

  const handleVelocityToggle = () => {
    usePianoRollStore.getState().toggleVelocityLane()
  }

  const handleQuantize = () => {
    quantizeSelectedNotes()
  }

  return (
    <div className="h-[36px] flex items-center gap-3 px-3 bg-[#252542] border-b border-[#3a3a5a] shrink-0">
      {/* Label */}
      <span className="text-[13px] font-medium text-[#eeeeee] select-none">Piano Roll</span>

      {/* Active clip name */}
      {clipName && (
        <span className="text-[13px] text-[#999999] select-none truncate max-w-[200px]">
          {clipName}
        </span>
      )}

      {/* Spacer */}
      <div className="flex-1" />

      {/* Velocity toggle */}
      <button
        onClick={handleVelocityToggle}
        className={`px-2 py-1 rounded text-[13px] transition-colors ${
          velocityLaneVisible
            ? 'text-[#6c63ff] underline'
            : 'text-[#999999] hover:bg-[#3a3a5a]'
        }`}
      >
        Velocity
      </button>

      {/* Quantize button */}
      <button
        onClick={handleQuantize}
        className="flex items-center gap-1 px-2 py-1 rounded text-[13px] text-[#eeeeee] hover:bg-[#3a3a5a] transition-colors"
        title="Snap selected notes to grid (Ctrl+Q)"
      >
        <Grid3x3 size={16} />
        Quantize
      </button>

      {/* Grid resolution */}
      <GridResolutionSelect />
    </div>
  )
}
