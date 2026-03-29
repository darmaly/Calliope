import { useTimelineStore } from '../../stores/timeline-store'
import { useMixerStore } from '../../stores/mixer-store'
import { useShallow } from 'zustand/shallow'
import { ChannelStrip } from './ChannelStrip'
import { MasterStrip } from './MasterStrip'

/**
 * Main mixer panel.
 * Displays horizontal row of channel strips (one per track) plus a master strip.
 * Scrolls horizontally when many tracks exist.
 */
export function MixerView() {
  const tracks = useTimelineStore(useShallow((s) => s.tracks))
  const mixerVisible = useMixerStore((s) => s.mixerVisible)
  const toggleMixerVisible = useMixerStore((s) => s.toggleMixerVisible)

  if (!mixerVisible) return null

  return (
    <div className="bg-[#1e1e3a] border-t border-[#3a3a5a] flex flex-col">
      {/* Mixer toolbar */}
      <div className="flex items-center justify-between px-3 py-1 bg-[#252542] border-b border-[#3a3a5a]">
        <span className="text-[12px] font-semibold text-[#cccccc] uppercase tracking-wider">
          Mixer
        </span>
        <button
          onClick={toggleMixerVisible}
          className="text-[11px] text-[#999999] hover:text-[#eeeeee] transition-colors"
        >
          Close
        </button>
      </div>

      {/* Channel strips container */}
      <div className="flex items-stretch overflow-x-auto p-2 gap-1.5">
        {/* Track strips */}
        {tracks
          .slice()
          .sort((a, b) => a.order - b.order)
          .map((track) => (
            <ChannelStrip key={track.id} track={track} />
          ))}

        {/* Empty state */}
        {tracks.length === 0 && (
          <div className="flex items-center justify-center text-[12px] text-[#666666] px-8 py-4">
            No tracks. Add tracks in the timeline to see mixer channels.
          </div>
        )}

        {/* Separator */}
        <div className="w-[1px] bg-[#4a4a6a] self-stretch mx-1 shrink-0" />

        {/* Master strip */}
        <MasterStrip />
      </div>
    </div>
  )
}
