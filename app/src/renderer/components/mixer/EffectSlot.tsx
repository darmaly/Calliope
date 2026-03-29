import { useState, useCallback } from 'react'
import { useContextMenu } from '../../hooks/use-context-menu'
import { ContextMenu } from '../shared/ContextMenu'
import { EFFECT_TYPES } from '../../types/mixer'
import type { EffectSlotInfo } from '../../types/mixer'

interface EffectSlotProps {
  slot: EffectSlotInfo | null
  index: number
  onAdd: (effectType: string) => void
  onRemove: () => void
  onBypass: () => void
  onClick: () => void
}

function effectLabel(effectType: string): string {
  const found = EFFECT_TYPES.find((e) => e.type === effectType)
  return found ? found.label : effectType
}

export function EffectSlot({ slot, index: _index, onAdd, onRemove, onBypass, onClick }: EffectSlotProps) {
  const [dropdownOpen, setDropdownOpen] = useState(false)
  const contextMenu = useContextMenu()

  const handleEmptyClick = useCallback(() => {
    setDropdownOpen((prev) => !prev)
  }, [])

  const handleContextMenu = useCallback(
    (e: React.MouseEvent) => {
      if (!slot) return
      contextMenu.show(e, [
        {
          label: slot.bypassed ? 'Enable Effect' : 'Bypass Effect',
          action: onBypass,
        },
        {
          label: 'Remove Effect',
          action: onRemove,
          destructive: true,
        },
      ])
    },
    [slot, contextMenu, onBypass, onRemove],
  )

  if (!slot) {
    return (
      <div
        style={{
          width: 56,
          height: 24,
          position: 'relative',
          cursor: 'pointer',
        }}
        onClick={handleEmptyClick}
      >
        <span style={{ fontSize: 10, color: '#666666', userSelect: 'none' }}>Add Effect</span>
        {dropdownOpen && (
          <div
            style={{
              position: 'absolute',
              top: 24,
              left: 0,
              width: 120,
              backgroundColor: '#252542',
              border: '1px solid #3a3a5a',
              borderRadius: 4,
              zIndex: 30,
              boxShadow: '0 4px 12px rgba(0,0,0,0.4)',
            }}
          >
            {EFFECT_TYPES.map((et) => (
              <button
                key={et.type}
                style={{
                  display: 'block',
                  width: '100%',
                  textAlign: 'left',
                  padding: '4px 8px',
                  fontSize: 11,
                  color: '#eeeeee',
                  background: 'none',
                  border: 'none',
                  cursor: 'pointer',
                }}
                onMouseEnter={(e) => {
                  ;(e.target as HTMLElement).style.backgroundColor = '#2a2a4a'
                }}
                onMouseLeave={(e) => {
                  ;(e.target as HTMLElement).style.backgroundColor = 'transparent'
                }}
                onClick={(e) => {
                  e.stopPropagation()
                  onAdd(et.type)
                  setDropdownOpen(false)
                }}
              >
                {et.label}
              </button>
            ))}
          </div>
        )}
      </div>
    )
  }

  return (
    <div
      style={{
        width: 56,
        height: 24,
        display: 'flex',
        alignItems: 'center',
        gap: 4,
        cursor: 'pointer',
      }}
      onClick={onClick}
      onContextMenu={handleContextMenu}
    >
      <span
        style={{
          fontSize: 10,
          color: slot.bypassed ? '#666666' : '#eeeeee',
          flex: 1,
          overflow: 'hidden',
          textOverflow: 'ellipsis',
          whiteSpace: 'nowrap',
          userSelect: 'none',
        }}
      >
        {effectLabel(slot.effectType)}
      </span>

      {/* Bypass toggle dot */}
      <button
        style={{
          width: 10,
          height: 10,
          borderRadius: '50%',
          backgroundColor: slot.bypassed ? 'transparent' : '#22c55e',
          border: `2px solid ${slot.bypassed ? '#3a3a5a' : '#22c55e'}`,
          cursor: 'pointer',
          padding: 0,
          flexShrink: 0,
        }}
        onClick={(e) => {
          e.stopPropagation()
          onBypass()
        }}
      />

      {contextMenu.visible && (
        <ContextMenu
          items={contextMenu.items}
          position={contextMenu.position}
          onClose={contextMenu.close}
        />
      )}
    </div>
  )
}
