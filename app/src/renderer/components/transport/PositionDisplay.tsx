import { useTimelineStore } from '../../stores/timeline-store'
import { useShallow } from 'zustand/shallow'
import { beatToTimeMs, formatTimeMs, beatToBarsBeats, formatBarsBeats } from '../../utils/time-format'

export function PositionDisplay() {
  const { currentBeat, bpm, beatsPerBar } = useTimelineStore(
    useShallow((s) => ({
      currentBeat: s.currentBeat,
      bpm: s.bpm,
      beatsPerBar: s.timeSignature.numerator,
    })),
  )

  const timeMs = beatToTimeMs(currentBeat, bpm)
  const timeStr = formatTimeMs(timeMs)
  const { bar, beatInBar } = beatToBarsBeats(currentBeat, beatsPerBar)
  const posStr = formatBarsBeats(bar, beatInBar)

  return (
    <div className="flex items-center gap-4">
      {/* Time display */}
      <div className="flex flex-col items-center" style={{ width: 120 }}>
        <span
          className="text-[16px] font-semibold text-[#eeeeee]"
          style={{ fontVariantNumeric: 'tabular-nums' }}
        >
          {timeStr}
        </span>
        <span className="text-[10px] text-[#999999] leading-tight">TIME</span>
      </div>

      {/* Vertical divider */}
      <div className="w-px h-5 bg-[#3a3a5a]" />

      {/* Bars:Beats display */}
      <div className="flex flex-col items-center" style={{ width: 96 }}>
        <span
          className="text-[16px] font-semibold text-[#eeeeee]"
          style={{ fontVariantNumeric: 'tabular-nums' }}
        >
          {posStr}
        </span>
        <span className="text-[10px] text-[#999999] leading-tight">POSITION</span>
      </div>
    </div>
  )
}
