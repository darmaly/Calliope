import { useMixerStore } from '../../stores/mixer-store'
import { MASTER_CHAIN } from '../../types/mixer'
import { EFFECTS, isEffectType } from '../../../shared/catalog'
import { ParamPanel } from '../shared/ParamPanel'

interface EffectParamPopoverProps {
  /** A track ID, or 'master'. */
  trackId: string
  slotIndex: number
  effectType: string
  position: { x: number; y: number }
  onClose: () => void
}

/** Settings for one effect slot. Values live in the mixer store; the engine follows. */
export function EffectParamPopover({ trackId, slotIndex, effectType, position, onClose }: EffectParamPopoverProps) {
  const slot = useMixerStore((s) =>
    trackId === MASTER_CHAIN ? s.masterEffects[slotIndex] : s.trackEffects[trackId]?.[slotIndex],
  )

  if (!slot || !isEffectType(effectType)) return null
  const spec = EFFECTS[effectType]

  return (
    <ParamPanel
      title={spec.label}
      specs={spec.params}
      values={slot.params}
      onChange={(name, value) => useMixerStore.getState().setEffectParam(trackId, slotIndex, name, value)}
      position={position}
      onClose={onClose}
    />
  )
}
