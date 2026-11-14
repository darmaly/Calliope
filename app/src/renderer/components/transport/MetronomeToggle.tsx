import { Timer } from 'lucide-react'
import { useTimelineStore } from '../../stores/timeline-store'

export function MetronomeToggle() {
  const metronomeEnabled = useTimelineStore((s) => s.metronomeEnabled)

  return (
    <button
      onPointerDown={() => useTimelineStore.getState().setMetronomeEnabled(!metronomeEnabled)}
      className={`w-8 h-8 flex items-center justify-center rounded-md transition-colors ${
        metronomeEnabled
          ? 'bg-[#6c63ff] text-white'
          : 'bg-[#3a3a5a] text-[#999999] hover:bg-[#4a4a6a] hover:text-[#eeeeee]'
      }`}
      title="Metronome"
      aria-pressed={metronomeEnabled}
    >
      <Timer size={16} />
    </button>
  )
}
