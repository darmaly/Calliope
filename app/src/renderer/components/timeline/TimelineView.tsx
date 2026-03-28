import { useRef } from 'react'
import { useTimelineStore } from '../../stores/timeline-store'
import { useShallow } from 'zustand/shallow'
import { TimelineToolbar } from './TimelineToolbar'
import { TimelineRuler } from './TimelineRuler'
import { TimelineCanvas } from './TimelineCanvas'
import { Playhead } from './Playhead'
import { TrackHeaderList } from '../tracks/TrackHeaderList'
import { AddTrackButton } from '../tracks/AddTrackButton'
import { usePlayhead } from '../../hooks/use-playhead'
import { useEngineSync } from '../../hooks/use-engine-sync'
import { useKeyboardShortcuts } from '../../hooks/use-keyboard-shortcuts'

export function TimelineView() {
  const canvasContainerRef = useRef<HTMLDivElement>(null)

  // Start rAF sync with engine transport
  usePlayhead()

  // Sync engine events to store
  useEngineSync()

  // Register global keyboard shortcuts
  useKeyboardShortcuts()

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

          {/* Canvas container */}
          <div
            ref={canvasContainerRef}
            className="flex-1 min-h-0 relative bg-[#1a1a2e]"
          >
            <TimelineCanvas containerRef={canvasContainerRef} />
          </div>
        </div>

        {/* Playhead overlay -- absolute positioned over entire content area */}
        <Playhead />
      </div>
    </div>
  )
}
