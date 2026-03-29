import { EffectSlot } from './EffectSlot'
import type { EffectSlotInfo } from '../../types/mixer'

interface EffectSlotListProps {
  slots: EffectSlotInfo[]
  onAdd: (effectType: string) => void
  onRemove: (index: number) => void
  onBypass: (index: number) => void
  onSlotClick: (index: number) => void
}

const MAX_SLOTS = 8

export function EffectSlotList({ slots, onAdd, onRemove, onBypass, onSlotClick }: EffectSlotListProps) {
  return (
    <div
      style={{
        display: 'flex',
        flexDirection: 'column',
        gap: 2,
        overflowY: 'auto',
        flex: 1,
        minHeight: 0,
      }}
    >
      {slots.map((slot, i) => (
        <EffectSlot
          key={i}
          slot={slot}
          index={i}
          onAdd={onAdd}
          onRemove={() => onRemove(i)}
          onBypass={() => onBypass(i)}
          onClick={() => onSlotClick(i)}
        />
      ))}
      {slots.length < MAX_SLOTS && (
        <EffectSlot
          slot={null}
          index={slots.length}
          onAdd={onAdd}
          onRemove={() => {}}
          onBypass={() => {}}
          onClick={() => {}}
        />
      )}
    </div>
  )
}
