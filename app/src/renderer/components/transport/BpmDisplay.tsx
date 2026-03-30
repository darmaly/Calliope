import { useState, useRef, useCallback, useEffect } from 'react'
import { useTimelineStore } from '../../stores/timeline-store'

export function BpmDisplay() {
  const bpm = useTimelineStore((s) => s.bpm)
  const [isEditing, setIsEditing] = useState(false)
  const [editValue, setEditValue] = useState('')
  const inputRef = useRef<HTMLInputElement>(null)

  const startEdit = useCallback(() => {
    setEditValue(String(Math.round(bpm)))
    setIsEditing(true)
  }, [bpm])

  useEffect(() => {
    if (isEditing && inputRef.current) {
      inputRef.current.focus()
      inputRef.current.select()
    }
  }, [isEditing])

  const commit = useCallback(() => {
    const val = Math.round(Number(editValue))
    if (val >= 20 && val <= 300) {
      window.calliope.setBpm(val)
    }
    setIsEditing(false)
  }, [editValue])

  const cancel = useCallback(() => {
    setIsEditing(false)
  }, [])

  const handleKeyDown = useCallback(
    (e: React.KeyboardEvent) => {
      if (e.key === 'Enter') {
        e.preventDefault()
        commit()
      } else if (e.key === 'Escape') {
        e.preventDefault()
        cancel()
      }
    },
    [commit, cancel],
  )

  return (
    <div className="flex flex-col items-center" style={{ width: 72 }}>
      {isEditing ? (
        <input
          ref={inputRef}
          type="number"
          min={20}
          max={300}
          step={1}
          value={editValue}
          onChange={(e) => setEditValue(e.target.value)}
          onKeyDown={handleKeyDown}
          onBlur={commit}
          className="w-full bg-[#1a1a2e] text-[#eeeeee] text-[16px] font-semibold text-center border border-[#6c63ff] rounded outline-none tabular-nums"
          style={{ fontVariantNumeric: 'tabular-nums' }}
        />
      ) : (
        <span
          className="text-[16px] font-semibold text-[#eeeeee] cursor-pointer tabular-nums"
          style={{ fontVariantNumeric: 'tabular-nums' }}
          onPointerDown={startEdit}
        >
          {Math.round(bpm)}
        </span>
      )}
      <span className="text-[10px] text-[#999999] leading-tight">BPM</span>
    </div>
  )
}
