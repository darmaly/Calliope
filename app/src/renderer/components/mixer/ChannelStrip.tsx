import { useCallback } from 'react'
import { useTimelineStore } from '../../stores/timeline-store'
import { useMixerStore } from '../../stores/mixer-store'
import { TRACK_COLORS } from '../../utils/colors'
import { Fader } from './Fader'
import { PanKnob } from './PanKnob'
import { LevelMeter } from './LevelMeter'
import { EffectSlot } from './EffectSlot'
import type { Track } from '../../types/timeline'

interface ChannelStripProps {
  track: Track
}

/**
 * Individual mixer channel strip for a track.
 * Contains: track name, effect inserts, pan knob, fader, level meter, mute/solo buttons.
 */
export function ChannelStrip({ track }: ChannelStripProps) {
  const toggleMute = useTimelineStore((s) => s.toggleMute)
  const toggleSolo = useTimelineStore((s) => s.toggleSolo)

  const volume = useMixerStore((s) => s.trackVolumes[track.id] ?? 1.0)
  const pan = useMixerStore((s) => s.trackPans[track.id] ?? 0.0)
  const levels = useMixerStore((s) => s.trackLevels[track.id] ?? { peakL: 0, peakR: 0 })
  const effects = useMixerStore((s) => s.trackEffects[track.id] ?? [])

  const setTrackVolume = useMixerStore((s) => s.setTrackVolume)
  const setTrackPan = useMixerStore((s) => s.setTrackPan)
  const addTrackEffect = useMixerStore((s) => s.addTrackEffect)
  const removeTrackEffect = useMixerStore((s) => s.removeTrackEffect)
  const bypassTrackEffect = useMixerStore((s) => s.bypassTrackEffect)
  const reorderTrackEffect = useMixerStore((s) => s.reorderTrackEffect)

  const color = TRACK_COLORS[track.colorIndex % TRACK_COLORS.length]

  const handleVolumeChange = useCallback(
    (v: number) => setTrackVolume(track.id, v),
    [track.id, setTrackVolume],
  )

  const handlePanChange = useCallback(
    (p: number) => setTrackPan(track.id, p),
    [track.id, setTrackPan],
  )

  return (
    <div className="flex flex-col items-center gap-1 w-[72px] min-w-[72px] bg-[#252542] rounded-md p-1.5 border border-[#3a3a5a]">
      {/* Track name */}
      <div
        className="w-full text-center text-[10px] truncate font-medium px-1"
        style={{ color }}
      >
        {track.name}
      </div>

      {/* Effect inserts */}
      <div className="w-full max-h-[80px] overflow-y-auto">
        <EffectSlot
          slots={effects}
          onAdd={(fx) => addTrackEffect(track.id, fx)}
          onRemove={(i) => removeTrackEffect(track.id, i)}
          onBypass={(i, b) => bypassTrackEffect(track.id, i, b)}
          onReorder={(from, to) => reorderTrackEffect(track.id, from, to)}
        />
      </div>

      {/* Pan knob */}
      <PanKnob value={pan} onChange={handlePanChange} size={28} color={color} />

      {/* Fader + Level meter side by side */}
      <div className="flex items-end gap-0.5">
        <Fader value={volume} onChange={handleVolumeChange} height={120} color={color} />
        <LevelMeter peakL={levels.peakL} peakR={levels.peakR} height={120} width={10} />
      </div>

      {/* Mute / Solo buttons */}
      <div className="flex gap-1">
        <button
          onClick={() => toggleMute(track.id)}
          className={`w-6 h-5 flex items-center justify-center rounded text-[10px] font-bold transition-colors ${
            track.muted
              ? 'bg-[#f59e0b] text-[#1a1a2e]'
              : 'bg-[#3a3a5a] text-[#999999] hover:text-[#eeeeee]'
          }`}
        >
          M
        </button>
        <button
          onClick={() => toggleSolo(track.id)}
          className={`w-6 h-5 flex items-center justify-center rounded text-[10px] font-bold transition-colors ${
            track.solo
              ? 'bg-[#22c55e] text-[#1a1a2e]'
              : 'bg-[#3a3a5a] text-[#999999] hover:text-[#eeeeee]'
          }`}
        >
          S
        </button>
      </div>

      {/* Color strip at bottom */}
      <div className="w-full h-[3px] rounded-full" style={{ backgroundColor: color }} />
    </div>
  )
}
