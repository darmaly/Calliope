import { Circle } from 'lucide-react'
import { useTimelineStore } from '../../stores/timeline-store'
import { TRACK_COLORS } from '../../utils/colors'
import type { Track } from '../../types/timeline'

interface TrackHeaderProps {
  track: Track
}

export function TrackHeader({ track }: TrackHeaderProps) {
  const toggleMute = useTimelineStore((s) => s.toggleMute)
  const toggleSolo = useTimelineStore((s) => s.toggleSolo)
  const toggleArm = useTimelineStore((s) => s.toggleArm)

  const color = TRACK_COLORS[track.colorIndex % TRACK_COLORS.length]

  return (
    <div
      className="flex items-center h-[80px] bg-[#252542] border-b border-[#3a3a5a]"
    >
      {/* Color strip */}
      <div
        className="w-[4px] h-full shrink-0"
        style={{ backgroundColor: color }}
      />

      <div className="flex flex-col justify-center gap-1.5 px-3 min-w-0 flex-1">
        {/* Track name */}
        <span className="text-[13px] text-[#eeeeee] truncate">{track.name}</span>

        {/* Buttons row */}
        <div className="flex items-center gap-1">
          {/* Mute */}
          <button
            onClick={() => toggleMute(track.id)}
            className={`w-6 h-6 flex items-center justify-center rounded text-[11px] font-bold transition-colors ${
              track.muted
                ? 'bg-[#f59e0b] text-[#1a1a2e]'
                : 'bg-[#3a3a5a] text-[#999999] hover:text-[#eeeeee]'
            }`}
          >
            M
          </button>

          {/* Solo */}
          <button
            onClick={() => toggleSolo(track.id)}
            className={`w-6 h-6 flex items-center justify-center rounded text-[11px] font-bold transition-colors ${
              track.solo
                ? 'bg-[#22c55e] text-[#1a1a2e]'
                : 'bg-[#3a3a5a] text-[#999999] hover:text-[#eeeeee]'
            }`}
          >
            S
          </button>

          {/* Arm */}
          <button
            onClick={() => toggleArm(track.id)}
            className={`w-6 h-6 flex items-center justify-center rounded transition-colors ${
              track.armed
                ? 'bg-[#ef4444] text-[#1a1a2e]'
                : 'bg-[#3a3a5a] text-[#999999] hover:text-[#eeeeee]'
            }`}
          >
            <Circle size={10} fill={track.armed ? '#1a1a2e' : 'none'} />
          </button>
        </div>
      </div>
    </div>
  )
}
