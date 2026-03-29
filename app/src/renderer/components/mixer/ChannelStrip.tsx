import { useState, useCallback } from 'react'
import { Circle } from 'lucide-react'
import { useMixerStore } from '../../stores/mixer-store'
import { useTimelineStore } from '../../stores/timeline-store'
import { Fader } from './Fader'
import { PanKnob } from './PanKnob'
import { LevelMeter } from './LevelMeter'
import { EffectSlotList } from './EffectSlotList'
import { EffectParamPopover } from './EffectParamPopover'
import type { Track } from '../../types/timeline'

interface ChannelStripProps {
  track: Track
}

export function ChannelStrip({ track }: ChannelStripProps) {
  const selectedTrackId = useTimelineStore((s) => s.selectedTrackId)
  const toggleMute = useTimelineStore((s) => s.toggleMute)
  const toggleSolo = useTimelineStore((s) => s.toggleSolo)
  const toggleArm = useTimelineStore((s) => s.toggleArm)
  const selectTrack = useTimelineStore((s) => s.selectTrack)

  const volume = useMixerStore((s) => s.trackVolumes[track.id] ?? 1.0)
  const pan = useMixerStore((s) => s.trackPans[track.id] ?? 0.0)
  const level = useMixerStore((s) => s.trackLevels[track.id])
  const effects = useMixerStore((s) => s.trackEffects[track.id] ?? [])

  const [popover, setPopover] = useState<{ index: number; x: number; y: number } | null>(null)

  const isSelected = selectedTrackId === track.id

  const handleVolumeChange = useCallback(
    (gain: number) => {
      useMixerStore.getState().setTrackVolume(track.id, gain)
      window.calliope.setTrackVolume(track.id, gain).catch(() => {})
    },
    [track.id],
  )

  const handlePanChange = useCallback(
    (newPan: number) => {
      useMixerStore.getState().setTrackPan(track.id, newPan)
      window.calliope.setTrackPan(track.id, newPan).catch(() => {})
    },
    [track.id],
  )

  const handleAddEffect = useCallback(
    (effectType: string) => {
      useMixerStore.getState().addTrackEffect(track.id, effectType)
      window.calliope.effectInsert(track.id, effectType).catch(() => {})
    },
    [track.id],
  )

  const handleRemoveEffect = useCallback(
    (index: number) => {
      useMixerStore.getState().removeTrackEffect(track.id, index)
      window.calliope.effectRemove(track.id, index).catch(() => {})
    },
    [track.id],
  )

  const handleBypassEffect = useCallback(
    (index: number) => {
      const slots = useMixerStore.getState().trackEffects[track.id] ?? []
      const slot = slots[index]
      if (!slot) return
      useMixerStore.getState().bypassTrackEffect(track.id, index, !slot.bypassed)
      window.calliope.effectBypass(track.id, index, !slot.bypassed).catch(() => {})
    },
    [track.id],
  )

  const handleSlotClick = useCallback(
    (index: number) => {
      const slots = useMixerStore.getState().trackEffects[track.id] ?? []
      const slot = slots[index]
      if (!slot) return
      // Position popover near the channel strip
      setPopover((prev) =>
        prev?.index === index ? null : { index, x: 80, y: 200 },
      )
    },
    [track.id],
  )

  return (
    <div
      style={{
        width: 64,
        minWidth: 64,
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        padding: 4,
        backgroundColor: '#252542',
        borderRight: '1px solid #3a3a5a',
        borderLeft: isSelected ? '2px solid #6c63ff' : '2px solid transparent',
        overflow: 'hidden',
        position: 'relative',
      }}
    >
      {/* Track name */}
      <div
        style={{
          width: '100%',
          height: 24,
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          cursor: 'pointer',
        }}
        onClick={() => selectTrack(track.id)}
      >
        <span
          style={{
            fontSize: 13,
            color: '#eeeeee',
            overflow: 'hidden',
            textOverflow: 'ellipsis',
            whiteSpace: 'nowrap',
            maxWidth: '100%',
            userSelect: 'none',
          }}
        >
          {track.name}
        </span>
      </div>

      {/* Level meter + Fader side by side */}
      <div style={{ display: 'flex', alignItems: 'flex-start', gap: 2, height: 180 }}>
        <LevelMeter
          leftLevel={level?.peakL ?? 0}
          rightLevel={level?.peakR ?? 0}
          leftPeak={level?.peakL ?? 0}
          rightPeak={level?.peakR ?? 0}
        />
        <Fader value={volume} onChange={handleVolumeChange} />
      </div>

      {/* Pan knob */}
      <PanKnob value={pan} onChange={handlePanChange} />

      {/* M / S / Arm buttons */}
      <div style={{ display: 'flex', gap: 2, marginTop: 4 }}>
        <button
          onClick={() => toggleMute(track.id)}
          className={`w-6 h-6 flex items-center justify-center rounded text-[11px] font-bold transition-colors ${
            track.muted
              ? 'bg-[#f59e0b] text-[#1a1a2e]'
              : 'bg-[#3a3a5a] text-[#999999] hover:text-[#eeeeee]'
          }`}
        >
          M
        </button>
        <button
          onClick={() => toggleSolo(track.id)}
          className={`w-6 h-6 flex items-center justify-center rounded text-[11px] font-bold transition-colors ${
            track.solo
              ? 'bg-[#22c55e] text-[#1a1a2e]'
              : 'bg-[#3a3a5a] text-[#999999] hover:text-[#eeeeee]'
          }`}
        >
          S
        </button>
        <button
          onClick={() => toggleArm(track.id)}
          className={`w-6 h-6 flex items-center justify-center rounded transition-colors ${
            track.armed
              ? 'bg-[#ef4444] text-[#1a1a2e]'
              : 'bg-[#3a3a5a] text-[#999999] hover:text-[#eeeeee]'
          }`}
        >
          <Circle size={10} fill={track.armed ? '#1a1a2e' : 'none'} />
        </button>
      </div>

      {/* Effect slot list */}
      <div style={{ marginTop: 4, width: '100%', flex: 1, minHeight: 0 }}>
        <EffectSlotList
          slots={effects}
          onAdd={handleAddEffect}
          onRemove={handleRemoveEffect}
          onBypass={handleBypassEffect}
          onSlotClick={handleSlotClick}
        />
      </div>

      {/* Effect param popover */}
      {popover && effects[popover.index] && (
        <EffectParamPopover
          trackId={track.id}
          slotIndex={popover.index}
          effectType={effects[popover.index].effectType}
          position={{ x: popover.x, y: popover.y }}
          onClose={() => setPopover(null)}
        />
      )}
    </div>
  )
}
