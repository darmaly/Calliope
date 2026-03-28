import { useState, useRef, useEffect } from 'react'
import { Circle } from 'lucide-react'
import { useTimelineStore } from '../../stores/timeline-store'
import { TRACK_COLORS } from '../../utils/colors'
import { useContextMenu } from '../../hooks/use-context-menu'
import { ContextMenu } from '../shared/ContextMenu'
import type { Track } from '../../types/timeline'

interface TrackHeaderProps {
  track: Track
}

export function TrackHeader({ track }: TrackHeaderProps) {
  const toggleMute = useTimelineStore((s) => s.toggleMute)
  const toggleSolo = useTimelineStore((s) => s.toggleSolo)
  const toggleArm = useTimelineStore((s) => s.toggleArm)
  const removeTrack = useTimelineStore((s) => s.removeTrack)
  const renameTrack = useTimelineStore((s) => s.renameTrack)
  const addTrack = useTimelineStore((s) => s.addTrack)
  const setTrackColor = useTimelineStore((s) => s.setTrackColor)

  const [editing, setEditing] = useState(false)
  const [editName, setEditName] = useState(track.name)
  const [showColorPicker, setShowColorPicker] = useState(false)
  const inputRef = useRef<HTMLInputElement>(null)
  const contextMenu = useContextMenu()

  const color = TRACK_COLORS[track.colorIndex % TRACK_COLORS.length]

  useEffect(() => {
    if (editing && inputRef.current) {
      inputRef.current.focus()
      inputRef.current.select()
    }
  }, [editing])

  const confirmRename = () => {
    const trimmed = editName.trim()
    if (trimmed && trimmed !== track.name) {
      renameTrack(track.id, trimmed)
    }
    setEditing(false)
  }

  const handleContextMenu = (e: React.MouseEvent) => {
    contextMenu.show(e, [
      {
        label: 'Add Track Above',
        action: () => addTrack(),
      },
      {
        label: 'Add Track Below',
        action: () => addTrack(),
      },
      {
        label: 'Rename Track',
        action: () => {
          setEditName(track.name)
          setEditing(true)
        },
      },
      {
        label: 'Change Color',
        action: () => setShowColorPicker(true),
      },
      {
        label: 'Duplicate Track',
        action: () => addTrack(track.name + ' Copy', track.colorIndex),
      },
      {
        label: 'Delete Track',
        action: () => removeTrack(track.id),
        destructive: true,
      },
    ])
  }

  const handleDoubleClick = () => {
    setEditName(track.name)
    setEditing(true)
  }

  return (
    <div
      className="flex items-center h-[80px] bg-[#252542] border-b border-[#3a3a5a] relative"
      onContextMenu={handleContextMenu}
    >
      {/* Color strip */}
      <div
        className="w-[4px] h-full shrink-0"
        style={{ backgroundColor: color }}
      />

      <div className="flex flex-col justify-center gap-1.5 px-3 min-w-0 flex-1">
        {/* Track name — editable on double-click */}
        {editing ? (
          <input
            ref={inputRef}
            value={editName}
            onChange={(e) => setEditName(e.target.value)}
            onBlur={confirmRename}
            onKeyDown={(e) => {
              if (e.key === 'Enter') confirmRename()
              if (e.key === 'Escape') setEditing(false)
            }}
            className="text-[13px] text-[#eeeeee] bg-[#1a1a2e] border border-[#6c63ff] rounded px-1 py-0 outline-none"
          />
        ) : (
          <span
            className="text-[13px] text-[#eeeeee] truncate cursor-default"
            onDoubleClick={handleDoubleClick}
          >
            {track.name}
          </span>
        )}

        {/* Buttons row */}
        <div className="flex items-center gap-1">
          {/* Mute */}
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

          {/* Solo */}
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

          {/* Arm */}
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
      </div>

      {/* Context menu */}
      {contextMenu.visible && (
        <ContextMenu
          items={contextMenu.items}
          position={contextMenu.position}
          onClose={contextMenu.close}
        />
      )}

      {/* Color picker overlay */}
      {showColorPicker && (
        <div
          className="absolute left-[200px] top-0 bg-[#252542] border border-[#3a3a5a] rounded-md shadow-lg p-2 z-50 grid grid-cols-4 gap-1"
          onMouseLeave={() => setShowColorPicker(false)}
        >
          {TRACK_COLORS.map((c, i) => (
            <button
              key={c}
              className="w-6 h-6 rounded-sm border border-transparent hover:border-white transition-colors"
              style={{ backgroundColor: c }}
              onClick={() => {
                setTrackColor(track.id, i)
                setShowColorPicker(false)
              }}
            />
          ))}
        </div>
      )}
    </div>
  )
}
