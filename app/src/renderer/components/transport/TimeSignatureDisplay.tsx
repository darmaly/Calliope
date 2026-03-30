import { useTimelineStore } from '../../stores/timeline-store'
import { useShallow } from 'zustand/shallow'

export function TimeSignatureDisplay() {
  const { numerator, denominator } = useTimelineStore(
    useShallow((s) => ({
      numerator: s.timeSignature.numerator,
      denominator: s.timeSignature.denominator,
    })),
  )

  return (
    <div className="flex flex-col items-center" style={{ width: 48 }}>
      <span
        className="text-[16px] font-semibold text-[#eeeeee]"
        style={{ fontVariantNumeric: 'tabular-nums' }}
      >
        {numerator}/{denominator}
      </span>
      <span className="text-[10px] text-[#999999] leading-tight">TIME SIG</span>
    </div>
  )
}
