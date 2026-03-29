import { useCallback } from 'react'
import { useMixerStore } from '../../stores/mixer-store'
import { Fader } from './Fader'
import { PanKnob } from './PanKnob'
import { LevelMeter } from './LevelMeter'
import { EffectSlot } from './EffectSlot'

/**
 * Master channel strip.
 * Visually wider than track strips, always visible at the right edge of the mixer.
 * Contains: master label, master effect inserts, pan knob, fader, level meter.
 */
export function MasterStrip() {
  const masterVolume = useMixerStore((s) => s.masterVolume)
  const masterPan = useMixerStore((s) => s.masterPan)
  const masterLevel = useMixerStore((s) => s.masterLevel)
  const masterEffects = useMixerStore((s) => s.masterEffects)

  const setMasterVolume = useMixerStore((s) => s.setMasterVolume)
  const setMasterPan = useMixerStore((s) => s.setMasterPan)
  const addMasterEffect = useMixerStore((s) => s.addMasterEffect)
  const removeMasterEffect = useMixerStore((s) => s.removeMasterEffect)
  const bypassMasterEffect = useMixerStore((s) => s.bypassMasterEffect)
  const reorderMasterEffect = useMixerStore((s) => s.reorderMasterEffect)

  const handleVolumeChange = useCallback(
    (v: number) => setMasterVolume(v),
    [setMasterVolume],
  )

  const handlePanChange = useCallback(
    (p: number) => setMasterPan(p),
    [setMasterPan],
  )

  const masterColor = '#ff6b6b'

  return (
    <div className="flex flex-col items-center gap-1 w-[88px] min-w-[88px] bg-[#2a2a4a] rounded-md p-1.5 border border-[#4a4a6a]">
      {/* Master label */}
      <div className="w-full text-center text-[11px] font-bold text-[#ff6b6b] uppercase tracking-wide">
        Master
      </div>

      {/* Master effect inserts */}
      <div className="w-full max-h-[80px] overflow-y-auto">
        <EffectSlot
          slots={masterEffects}
          onAdd={(fx) => addMasterEffect(fx)}
          onRemove={(i) => removeMasterEffect(i)}
          onBypass={(i, b) => bypassMasterEffect(i, b)}
          onReorder={(from, to) => reorderMasterEffect(from, to)}
        />
      </div>

      {/* Pan knob */}
      <PanKnob value={masterPan} onChange={handlePanChange} size={32} color={masterColor} />

      {/* Fader + Level meter */}
      <div className="flex items-end gap-1">
        <Fader value={masterVolume} onChange={handleVolumeChange} height={140} color={masterColor} />
        <LevelMeter
          peakL={masterLevel.peakL}
          peakR={masterLevel.peakR}
          height={140}
          width={14}
        />
      </div>

      {/* Master color strip */}
      <div className="w-full h-[3px] rounded-full bg-[#ff6b6b]" />
    </div>
  )
}
