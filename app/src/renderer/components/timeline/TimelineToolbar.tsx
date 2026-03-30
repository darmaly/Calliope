import { ZoomIn, ZoomOut } from 'lucide-react'
import { useTimelineStore } from '../../stores/timeline-store'
import { useShallow } from 'zustand/shallow'
import { GridResolutionSelect } from '../shared/GridResolutionSelect'

const MIN_PPB = 6
const MAX_PPB = 192
const ZOOM_FACTOR = 1.5

export function TimelineToolbar() {
  const { snapEnabled, pixelsPerBeat } = useTimelineStore(
    useShallow((s) => ({
      snapEnabled: s.snapEnabled,
      pixelsPerBeat: s.pixelsPerBeat,
    }))
  )
  const toggleSnap = useTimelineStore((s) => s.toggleSnap)
  const setPixelsPerBeat = useTimelineStore((s) => s.setPixelsPerBeat)

  const zoomIn = () => {
    const next = Math.min(pixelsPerBeat * ZOOM_FACTOR, MAX_PPB)
    setPixelsPerBeat(next)
  }

  const zoomOut = () => {
    const next = Math.max(pixelsPerBeat / ZOOM_FACTOR, MIN_PPB)
    setPixelsPerBeat(next)
  }

  return (
    <div className="h-[48px] flex items-center gap-3 px-4 bg-[#252542] border-b border-[#3a3a5a] shrink-0">
      <GridResolutionSelect />

      {/* Snap toggle */}
      <button
        onClick={toggleSnap}
        className={`px-2.5 py-1 rounded text-[13px] font-medium transition-colors ${
          snapEnabled
            ? 'bg-[#6c63ff] text-white'
            : 'bg-[#3a3a5a] text-[#999999] hover:text-[#eeeeee]'
        }`}
      >
        Snap
      </button>

      {/* Separator */}
      <div className="w-px h-5 bg-[#3a3a5a]" />

      {/* Zoom controls */}
      <div className="flex items-center gap-1">
        <button
          onClick={zoomOut}
          className="p-1.5 rounded text-[#999999] hover:text-[#eeeeee] hover:bg-[#3a3a5a] transition-colors"
          title="Zoom Out"
        >
          <ZoomOut size={16} />
        </button>
        <button
          onClick={zoomIn}
          className="p-1.5 rounded text-[#999999] hover:text-[#eeeeee] hover:bg-[#3a3a5a] transition-colors"
          title="Zoom In"
        >
          <ZoomIn size={16} />
        </button>
      </div>
    </div>
  )
}
