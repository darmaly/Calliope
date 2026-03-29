import type { EffectSlotInfo } from '../../types/mixer'

const EFFECT_TYPES = ['eq', 'compressor', 'reverb', 'delay', 'limiter'] as const

interface EffectSlotProps {
  slots: EffectSlotInfo[]
  onAdd: (effectType: string) => void
  onRemove: (index: number) => void
  onBypass: (index: number, bypassed: boolean) => void
  onReorder: (fromIndex: number, toIndex: number) => void
  maxSlots?: number
}

/**
 * Effect insert chain display for a channel strip.
 * Shows a list of effect slots with add/remove/bypass controls.
 */
export function EffectSlot({
  slots,
  onAdd,
  onRemove,
  onBypass,
  onReorder,
  maxSlots = 8,
}: EffectSlotProps) {
  return (
    <div className="flex flex-col gap-0.5 w-full">
      {/* Existing slots */}
      {slots.map((slot, index) => (
        <div
          key={`${slot.effectType}-${index}`}
          className={`flex items-center gap-0.5 px-1 py-0.5 rounded text-[9px] ${
            slot.bypassed ? 'bg-[#2a2a4a] text-[#666666]' : 'bg-[#3a3a5a] text-[#cccccc]'
          }`}
          draggable
          onDragStart={(e) => {
            e.dataTransfer.setData('text/plain', String(index))
          }}
          onDragOver={(e) => e.preventDefault()}
          onDrop={(e) => {
            e.preventDefault()
            const fromStr = e.dataTransfer.getData('text/plain')
            const from = parseInt(fromStr, 10)
            if (!isNaN(from) && from !== index) {
              onReorder(from, index)
            }
          }}
        >
          {/* Effect name */}
          <span className="flex-1 truncate uppercase">{slot.effectType}</span>
          {/* Bypass toggle */}
          <button
            className={`w-3 h-3 rounded-full border ${
              slot.bypassed
                ? 'border-[#666666] bg-transparent'
                : 'border-[#22c55e] bg-[#22c55e]'
            }`}
            onClick={() => onBypass(index, !slot.bypassed)}
            title={slot.bypassed ? 'Enable' : 'Bypass'}
          />
          {/* Remove button */}
          <button
            className="text-[#999999] hover:text-[#ef4444] text-[8px] leading-none"
            onClick={() => onRemove(index)}
            title="Remove"
          >
            x
          </button>
        </div>
      ))}

      {/* Add button */}
      {slots.length < maxSlots && (
        <div className="relative group">
          <button
            className="w-full text-[9px] text-[#666666] hover:text-[#999999] border border-dashed border-[#3a3a5a] hover:border-[#666666] rounded px-1 py-0.5 transition-colors"
          >
            + FX
          </button>
          {/* Dropdown on hover */}
          <div className="absolute bottom-full left-0 w-full bg-[#252542] border border-[#3a3a5a] rounded shadow-lg hidden group-hover:block z-50">
            {EFFECT_TYPES.map((fx) => (
              <button
                key={fx}
                className="block w-full text-left px-2 py-1 text-[9px] text-[#cccccc] hover:bg-[#3a3a5a] uppercase"
                onClick={() => onAdd(fx)}
              >
                {fx}
              </button>
            ))}
          </div>
        </div>
      )}
    </div>
  )
}
