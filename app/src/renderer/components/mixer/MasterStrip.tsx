import { useState, useCallback } from 'react'
import { useMixerStore } from '../../stores/mixer-store'
import { Fader } from './Fader'
import { LevelMeter } from './LevelMeter'
import { EffectSlotList } from './EffectSlotList'
import { EffectParamPopover } from './EffectParamPopover'

export function MasterStrip() {
  const masterVolume = useMixerStore((s) => s.masterVolume)
  const masterLevel = useMixerStore((s) => s.masterLevel)
  const masterEffects = useMixerStore((s) => s.masterEffects)

  const [popover, setPopover] = useState<{ index: number; x: number; y: number } | null>(null)

  const handleVolumeChange = useCallback((gain: number) => {
    useMixerStore.getState().setMasterVolume(gain)
    window.calliope
      .dispatchCommand({ command: 'master.setVolume', params: { volume: gain } })
      .catch(() => {})
  }, [])

  const handleAddEffect = useCallback((effectType: string) => {
    useMixerStore.getState().addMasterEffect(effectType)
    window.calliope.effectInsert('master', effectType).catch(() => {})
  }, [])

  const handleRemoveEffect = useCallback((index: number) => {
    useMixerStore.getState().removeMasterEffect(index)
    window.calliope.effectRemove('master', index).catch(() => {})
  }, [])

  const handleBypassEffect = useCallback((index: number) => {
    const effects = useMixerStore.getState().masterEffects
    const slot = effects[index]
    if (!slot) return
    useMixerStore.getState().bypassMasterEffect(index, !slot.bypassed)
    window.calliope.effectBypass('master', index, !slot.bypassed).catch(() => {})
  }, [])

  const handleSlotClick = useCallback(
    (index: number) => {
      const effects = useMixerStore.getState().masterEffects
      const slot = effects[index]
      if (!slot) return
      setPopover((prev) =>
        prev?.index === index ? null : { index, x: 80, y: 200 },
      )
    },
    [],
  )

  return (
    <div
      style={{
        width: 88,
        minWidth: 88,
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        padding: 4,
        backgroundColor: '#2a2a4e',
        borderLeft: '2px solid #6c63ff',
        overflow: 'hidden',
        position: 'relative',
      }}
    >
      {/* Master label */}
      <div
        style={{
          width: '100%',
          height: 24,
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
        }}
      >
        <span
          style={{
            fontSize: 13,
            fontWeight: 500,
            color: '#eeeeee',
            userSelect: 'none',
          }}
        >
          Master
        </span>
      </div>

      {/* Level meter + Fader side by side */}
      <div style={{ display: 'flex', alignItems: 'flex-start', gap: 2, height: 180 }}>
        <LevelMeter
          leftLevel={masterLevel.peakL}
          rightLevel={masterLevel.peakR}
          leftPeak={masterLevel.peakL}
          rightPeak={masterLevel.peakR}
        />
        <Fader value={masterVolume} onChange={handleVolumeChange} />
      </div>

      {/* No pan or M/S/Arm for master */}

      {/* Effect slot list */}
      <div style={{ marginTop: 8, width: '100%', flex: 1, minHeight: 0 }}>
        <EffectSlotList
          slots={masterEffects}
          onAdd={handleAddEffect}
          onRemove={handleRemoveEffect}
          onBypass={handleBypassEffect}
          onSlotClick={handleSlotClick}
        />
      </div>

      {/* Effect param popover */}
      {popover && masterEffects[popover.index] && (
        <EffectParamPopover
          trackId="master"
          slotIndex={popover.index}
          effectType={masterEffects[popover.index].effectType}
          position={{ x: popover.x, y: popover.y }}
          onClose={() => setPopover(null)}
        />
      )}
    </div>
  )
}
