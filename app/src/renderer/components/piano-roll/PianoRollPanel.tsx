import { usePianoRollStore } from '../../stores/piano-roll-store'
import { PianoRollToolbar } from './PianoRollToolbar'

export function PianoRollPanel() {
  const activeClipId = usePianoRollStore((s) => s.activeClipId)

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

  return (
    <div className="h-full flex flex-col bg-[#1a1a2e]">
      <PianoRollToolbar />
      {/* Canvas placeholder — will be replaced in Task 2 */}
      <div className="flex-1 min-h-0 relative" />
    </div>
  )
}
