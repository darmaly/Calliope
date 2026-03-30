import { useState } from 'react'
import { Timer } from 'lucide-react'

export function MetronomeToggle() {
  const [metronomeEnabled, setMetronomeEnabled] = useState(false)

  const handleToggle = () => {
    const next = !metronomeEnabled
    setMetronomeEnabled(next)
    window.calliope.setMetronomeEnabled?.(next)
  }

  return (
    <button
      onPointerDown={handleToggle}
      className={`w-8 h-8 flex items-center justify-center rounded-md transition-colors ${
        metronomeEnabled
          ? 'bg-[#6c63ff] text-white'
          : 'bg-[#3a3a5a] text-[#999999] hover:bg-[#4a4a6a] hover:text-[#eeeeee]'
      }`}
      title="Metronome"
    >
      <Timer size={16} />
    </button>
  )
}
