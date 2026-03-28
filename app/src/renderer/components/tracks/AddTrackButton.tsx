import { Plus } from 'lucide-react'
import { useTimelineStore } from '../../stores/timeline-store'

export function AddTrackButton() {
  const addTrack = useTimelineStore((s) => s.addTrack)

  return (
    <button
      onClick={() => addTrack()}
      className="flex items-center gap-2 w-full px-3 py-2 text-[13px] text-[#999999] hover:text-[#eeeeee] hover:bg-[#2a2a4a] transition-colors"
    >
      <Plus size={16} />
      <span>Add Track</span>
    </button>
  )
}
