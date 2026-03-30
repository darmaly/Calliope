import { useState } from 'react'
import { Repeat } from 'lucide-react'

export function LoopToggle() {
  const [loopEnabled, setLoopEnabled] = useState(false)

  const handleToggle = () => {
    const next = !loopEnabled
    setLoopEnabled(next)
    window.calliope.setLoopRegion?.(0, 0, next)
  }

  return (
    <button
      onPointerDown={handleToggle}
      className={`w-8 h-8 flex items-center justify-center rounded-md transition-colors ${
        loopEnabled
          ? 'bg-[#6c63ff] text-white'
          : 'bg-[#3a3a5a] text-[#999999] hover:bg-[#4a4a6a] hover:text-[#eeeeee]'
      }`}
      title="Loop (L)"
    >
      <Repeat size={16} />
    </button>
  )
}
