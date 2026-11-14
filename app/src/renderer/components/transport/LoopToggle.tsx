import { Repeat } from 'lucide-react'
import { useTimelineStore } from '../../stores/timeline-store'

/**
 * Switches looping of the region drawn on the ruler on and off.
 * With no region there is nothing to loop, so the button is disabled.
 */
export function LoopToggle() {
  const loopEnabled = useTimelineStore((s) => s.loopEnabled)
  const hasRegion = useTimelineStore((s) => s.loopRegion !== null)

  return (
    <button
      onPointerDown={() => {
        if (hasRegion) useTimelineStore.getState().setLoopEnabled(!loopEnabled)
      }}
      disabled={!hasRegion}
      className={`w-8 h-8 flex items-center justify-center rounded-md transition-colors disabled:opacity-40 disabled:cursor-not-allowed ${
        loopEnabled && hasRegion
          ? 'bg-[#6c63ff] text-white'
          : 'bg-[#3a3a5a] text-[#999999] hover:bg-[#4a4a6a] hover:text-[#eeeeee]'
      }`}
      title={hasRegion ? 'Loop (L)' : 'Drag on the ruler to set a loop region'}
      aria-pressed={loopEnabled && hasRegion}
    >
      <Repeat size={16} />
    </button>
  )
}
