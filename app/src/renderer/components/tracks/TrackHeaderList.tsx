import { useTimelineStore } from '../../stores/timeline-store'
import { TrackHeader } from './TrackHeader'

export function TrackHeaderList() {
  const tracks = useTimelineStore((s) => s.tracks)
  const scrollY = useTimelineStore((s) => s.scrollY)

  return (
    <div className="flex-1 overflow-hidden">
      <div
        className="flex flex-col"
        style={{ transform: `translateY(${-scrollY}px)` }}
      >
        {tracks.map((track) => (
          <TrackHeader key={track.id} track={track} />
        ))}
      </div>
    </div>
  )
}
