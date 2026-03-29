import { useState, useCallback } from 'react'
import { Grid3x3, ZoomIn, ZoomOut } from 'lucide-react'
import { usePianoRollStore } from '../../stores/piano-roll-store'
import { useTimelineStore } from '../../stores/timeline-store'
import { useShallow } from 'zustand/shallow'
import { GridResolutionSelect } from '../shared/GridResolutionSelect'
import { quantizeSelectedNotes } from '../../utils/note-operations'

export function PianoRollToolbar() {
  const { activeClipId, velocityLaneVisible, selectedNoteIds } = usePianoRollStore(
    useShallow((s) => ({
      activeClipId: s.activeClipId,
      velocityLaneVisible: s.velocityLaneVisible,
      selectedNoteIds: s.selectedNoteIds,
    })),
  )

  const [toast, setToast] = useState<string | null>(null)

  const clips = useTimelineStore((s) => s.clips)
  const clipName = activeClipId ? clips[activeClipId]?.name ?? 'Untitled Clip' : ''

  const handleVelocityToggle = () => {
    usePianoRollStore.getState().toggleVelocityLane()
  }

  const showToast = useCallback((msg: string) => {
    setToast(msg)
    setTimeout(() => setToast(null), 2000)
  }, [])

  const handleQuantize = () => {
    if (selectedNoteIds.size === 0) {
      showToast('Select notes to quantize')
      return
    }
    quantizeSelectedNotes()
    showToast(`Quantized ${selectedNoteIds.size} note${selectedNoteIds.size > 1 ? 's' : ''}`)
  }

  return (
    <div className="h-[36px] flex items-center gap-3 px-3 bg-[#252542] border-b border-[#3a3a5a] shrink-0 relative">
      {/* Label */}
      <span className="text-[13px] font-medium text-[#eeeeee] select-none">Piano Roll</span>

      {/* Active clip name */}
      {clipName && (
        <span className="text-[13px] text-[#999999] select-none truncate max-w-[200px]">
          {clipName}
        </span>
      )}

      {/* Spacer */}
      <div className="flex-1" />

      {/* Velocity toggle */}
      <button
        onClick={handleVelocityToggle}
        className={`px-2 py-1 rounded text-[13px] transition-colors ${
          velocityLaneVisible
            ? 'text-[#6c63ff] underline'
            : 'text-[#999999] hover:bg-[#3a3a5a]'
        }`}
      >
        Velocity
      </button>

      {/* Quantize button */}
      <button
        onClick={handleQuantize}
        className="flex items-center gap-1 px-2 py-1 rounded text-[13px] text-[#eeeeee] hover:bg-[#3a3a5a] transition-colors"
        title="Snap selected notes to grid (Ctrl+Q)"
      >
        <Grid3x3 size={16} />
        Quantize
      </button>

      {/* Vertical zoom buttons */}
      <button
        onClick={() => {
          const h = usePianoRollStore.getState().noteRowHeight
          usePianoRollStore.getState().setNoteRowHeight(Math.min(48, Math.round(h * 1.3)))
        }}
        className="p-1 rounded text-[#999999] hover:bg-[#3a3a5a] transition-colors"
        title="Zoom in vertically (Ctrl+Shift+Scroll Up)"
      >
        <ZoomIn size={16} />
      </button>
      <button
        onClick={() => {
          const h = usePianoRollStore.getState().noteRowHeight
          usePianoRollStore.getState().setNoteRowHeight(Math.max(4, Math.round(h / 1.3)))
        }}
        className="p-1 rounded text-[#999999] hover:bg-[#3a3a5a] transition-colors"
        title="Zoom out vertically (Ctrl+Shift+Scroll Down)"
      >
        <ZoomOut size={16} />
      </button>

      {/* Grid resolution */}
      <GridResolutionSelect />

      {/* Toast notification */}
      {toast && (
        <span className="absolute top-[40px] right-3 bg-[#252542] border border-[#3a3a5a] rounded px-3 py-1 text-[10px] text-[#eeeeee] z-20 animate-pulse">
          {toast}
        </span>
      )}
    </div>
  )
}
