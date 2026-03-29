import { useTimelineStore } from '../../stores/timeline-store'
import { useMeterPolling } from '../../hooks/use-meter-polling'
import { MixerToolbar } from './MixerToolbar'
import { ChannelStrip } from './ChannelStrip'
import { MasterStrip } from './MasterStrip'

export function MixerPanel() {
  useMeterPolling()

  const tracks = useTimelineStore((s) => s.tracks)

  if (tracks.length === 0) {
    return (
      <div
        style={{
          height: '100%',
          display: 'flex',
          flexDirection: 'column',
          backgroundColor: '#1a1a2e',
        }}
      >
        <MixerToolbar />
        <div
          style={{
            flex: 1,
            display: 'flex',
            flexDirection: 'column',
            alignItems: 'center',
            justifyContent: 'center',
          }}
        >
          <p style={{ fontSize: 13, fontWeight: 500, color: '#eeeeee' }}>
            No tracks to mix
          </p>
          <p style={{ fontSize: 13, color: '#999999', marginTop: 4 }}>
            Add a track in the timeline to see it here.
          </p>
        </div>
      </div>
    )
  }

  return (
    <div
      style={{
        height: '100%',
        display: 'flex',
        flexDirection: 'column',
        backgroundColor: '#1a1a2e',
      }}
    >
      <MixerToolbar />
      <div style={{ flex: 1, display: 'flex', minHeight: 0 }}>
        {/* Channel strip scroller */}
        <div
          style={{
            flex: 1,
            overflowX: 'auto',
            overflowY: 'hidden',
            display: 'flex',
          }}
        >
          {tracks.map((track) => (
            <ChannelStrip key={track.id} track={track} />
          ))}
        </div>
        {/* Master strip pinned right */}
        <MasterStrip />
      </div>
    </div>
  )
}
