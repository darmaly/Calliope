import { useTimelineStore } from '../../stores/timeline-store'
import type { GridResolution } from '../../types/timeline'

const GRID_OPTIONS: { label: string; value: GridResolution }[] = [
  { label: '1/4', value: 0.25 },
  { label: '1/4T', value: 0.16667 },
  { label: '1/8', value: 0.125 },
  { label: '1/8T', value: 0.08333 },
  { label: '1/16', value: 0.0625 },
  { label: '1/16T', value: 0.04167 },
  { label: '1/32', value: 0.03125 },
]

export function GridResolutionSelect() {
  const gridResolution = useTimelineStore((s) => s.gridResolution)
  const setGridResolution = useTimelineStore((s) => s.setGridResolution)

  return (
    <div className="flex items-center gap-1.5">
      <label className="text-[11px] text-[#999999] select-none">Grid</label>
      <select
        value={gridResolution}
        onChange={(e) => setGridResolution(Number(e.target.value) as GridResolution)}
        className="bg-[#252542] text-[#eeeeee] border border-[#3a3a5a] rounded px-1.5 py-0.5 text-[13px] outline-none focus:border-[#6c63ff]"
      >
        {GRID_OPTIONS.map((opt) => (
          <option key={opt.value} value={opt.value}>
            {opt.label}
          </option>
        ))}
      </select>
    </div>
  )
}
