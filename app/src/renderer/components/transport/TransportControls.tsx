import { useState } from 'react'
import { Play, Pause, Square } from 'lucide-react'
import { useTimelineStore } from '../../stores/timeline-store'

export function TransportControls() {
  const isPlaying = useTimelineStore((s) => s.isPlaying)

  const handlePlayPause = () => {
    if (isPlaying) {
      window.calliope.transportStop()
    } else {
      window.calliope.transportPlay()
    }
  }

  const handleStop = () => {
    window.calliope.transportStop()
  }

  return (
    <div className="flex items-center gap-1">
      {/* Play / Pause */}
      <button
        onPointerDown={handlePlayPause}
        className={`w-8 h-8 flex items-center justify-center rounded-md transition-colors ${
          isPlaying
            ? 'bg-[#6c63ff] text-white'
            : 'bg-[#3a3a5a] text-[#eeeeee] hover:bg-[#4a4a6a]'
        }`}
        title={isPlaying ? 'Stop (Space)' : 'Play (Space)'}
      >
        {isPlaying ? <Pause size={16} /> : <Play size={16} />}
      </button>

      {/* Stop */}
      <button
        onPointerDown={handleStop}
        className="w-8 h-8 flex items-center justify-center rounded-md bg-[#3a3a5a] text-[#eeeeee] hover:bg-[#4a4a6a] transition-colors"
        title="Stop (Space)"
      >
        <Square size={16} />
      </button>

      {/* Record */}
      <RecordButton />
    </div>
  )
}

function RecordButton() {
  // Placeholder: record arm state is local until engine support is added
  const [isArmed, setIsArmed] = useState(false)

  return (
    <button
      onPointerDown={() => setIsArmed((v) => !v)}
      className={`w-8 h-8 flex items-center justify-center rounded-md transition-colors ${
        isArmed
          ? 'bg-[#ef4444] animate-pulse'
          : 'bg-[#3a3a5a] hover:bg-[#4a4a6a]'
      }`}
      title="Record (R)"
    >
      <div
        className={`w-3 h-3 rounded-full ${isArmed ? 'bg-white' : 'bg-[#999999]'}`}
      />
    </button>
  )
}
