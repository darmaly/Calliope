import { useRef } from 'react'
import { useTimelineStore } from '../../stores/timeline-store'
import { useShallow } from 'zustand/shallow'
import { TimelineToolbar } from './TimelineToolbar'
import { TimelineRuler } from './TimelineRuler'
import { TrackHeaderList } from '../tracks/TrackHeaderList'
import { AddTrackButton } from '../tracks/AddTrackButton'

export function TimelineView() {
  const canvasContainerRef = useRef<HTMLDivElement>(null)

  const { scrollY, tracks } = useTimelineStore(
    useShallow((s) => ({
      scrollY: s.scrollY,
      tracks: s.tracks,
    }))
  )

  return (
    <div className="flex flex-col h-full w-full bg-[#1a1a2e]">
      {/* Toolbar */}
      <TimelineToolbar />

      {/* Content area: track headers + canvas */}
      <div className="flex flex-1 min-h-0 relative">
        {/* Track header column */}
        <div className="w-[200px] shrink-0 flex flex-col border-r border-[#3a3a5a] bg-[#1a1a2e]">
          {/* Ruler spacer for alignment */}
          <div className="h-[48px] shrink-0 bg-[#252542] border-b border-[#3a3a5a]" />

          {/* Track headers */}
          <TrackHeaderList />

          {/* Add track button */}
          <AddTrackButton />
        </div>

        {/* Right column: ruler + canvas */}
        <div className="flex flex-col flex-1 min-w-0">
          {/* Ruler */}
          <TimelineRuler />

          {/* Canvas container - placeholder for TimelineCanvas (Task 2) */}
          <div
            ref={canvasContainerRef}
            className="flex-1 min-h-0 relative bg-[#1a1a2e]"
          >
            {/* TimelineCanvas and Playhead will be wired in Task 2 */}
          </div>
        </div>
      </div>
    </div>
  )
}
