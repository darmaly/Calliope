import { useState } from 'react'
import { Plus } from 'lucide-react'
import { useTimelineStore } from '../../stores/timeline-store'
import { INSTRUMENTS, INSTRUMENT_TYPES } from '../../../shared/catalog'
import type { InstrumentType } from '../../../shared/catalog'

/** Adds a track. Every track plays through an instrument, so the instrument is chosen here. */
export function AddTrackButton() {
  const [open, setOpen] = useState(false)

  const add = (instrument: InstrumentType) => {
    const store = useTimelineStore.getState()
    const sameKind = store.tracks.filter((t) => t.instrument === instrument).length
    const label = INSTRUMENTS[instrument].label
    const id = store.addTrack(sameKind === 0 ? label : `${label} ${sameKind + 1}`, undefined, instrument)
    store.selectTrack(id)
    setOpen(false)
  }

  return (
    <div className="relative w-full" onMouseLeave={() => setOpen(false)}>
      <button
        onClick={() => setOpen((v) => !v)}
        className="flex items-center gap-2 w-full px-3 py-2 text-[13px] text-[#999999] hover:text-[#eeeeee] hover:bg-[#2a2a4a] transition-colors"
        aria-haspopup="menu"
        aria-expanded={open}
      >
        <Plus size={16} />
        <span>Add Track</span>
      </button>
      {open && (
        <div
          role="menu"
          className="absolute left-2 top-full z-40 w-[184px] rounded-md border border-[#3a3a5a] bg-[#252542] py-1 shadow-lg"
        >
          {INSTRUMENT_TYPES.map((type) => (
            <button
              key={type}
              role="menuitem"
              onClick={() => add(type)}
              title={INSTRUMENTS[type].description}
              className="block w-full px-3 py-1.5 text-left text-[13px] text-[#eeeeee] hover:bg-[#3a3a5a]"
            >
              {INSTRUMENTS[type].label}
            </button>
          ))}
        </div>
      )}
    </div>
  )
}
