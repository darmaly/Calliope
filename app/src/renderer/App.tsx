import { useState, useEffect } from 'react'
import { TimelineView } from './components/timeline/TimelineView'
import './App.css'

export default function App() {
  const [engineStatus, setEngineStatus] = useState<string>('connecting...')

  useEffect(() => {
    window.calliope.getEngineInfo()
      .then((info) => setEngineStatus(`JUCE ${info.juceVersion}`))
      .catch(() => setEngineStatus('offline'))
  }, [])

  return (
    <div className="h-screen w-screen bg-[#1a1a2e] text-[#eeeeee] overflow-hidden flex flex-col">
      <TimelineView />
      {/* Engine status indicator */}
      <div className="absolute bottom-1 right-2 text-[10px] text-[#666666] pointer-events-none select-none">
        Engine: {engineStatus}
      </div>
    </div>
  )
}
