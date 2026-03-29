import { useState, useCallback } from 'react'
import { EffectSlot } from './EffectSlot'
import type { EffectSlotInfo } from '../../types/mixer'

interface EffectSlotListProps {
  slots: EffectSlotInfo[]
  onAdd: (effectType: string) => void
  onRemove: (index: number) => void
  onBypass: (index: number) => void
  onSlotClick: (index: number) => void
  onReorder: (fromIndex: number, toIndex: number) => void
}

const MAX_SLOTS = 8

export function EffectSlotList({ slots, onAdd, onRemove, onBypass, onSlotClick, onReorder }: EffectSlotListProps) {
  const [draggedIndex, setDraggedIndex] = useState<number | null>(null)
  const [dragOverIndex, setDragOverIndex] = useState<number | null>(null)

  const handleDragStart = useCallback((index: number, e: React.DragEvent) => {
    setDraggedIndex(index)
    e.dataTransfer.effectAllowed = 'move'
    e.dataTransfer.setData('text/plain', String(index))
  }, [])

  const handleDragOver = useCallback((index: number, e: React.DragEvent) => {
    e.preventDefault()
    setDragOverIndex(index)
  }, [])

  const handleDrop = useCallback((targetIndex: number, e: React.DragEvent) => {
    e.preventDefault()
    if (draggedIndex !== null && draggedIndex !== targetIndex) {
      onReorder(draggedIndex, targetIndex)
    }
    setDraggedIndex(null)
    setDragOverIndex(null)
  }, [draggedIndex, onReorder])

  const handleDragEnd = useCallback(() => {
    setDraggedIndex(null)
    setDragOverIndex(null)
  }, [])

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
          draggable={true}
          onDragStart={(e) => handleDragStart(i, e)}
          onDragOver={(e) => handleDragOver(i, e)}
          onDrop={(e) => handleDrop(i, e)}
          onDragEnd={handleDragEnd}
          isDragOver={dragOverIndex === i && draggedIndex !== i}
          isDragging={draggedIndex === i}
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
