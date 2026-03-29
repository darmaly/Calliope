import { useState, useEffect, useCallback } from 'react'
import { TimelineView } from './components/timeline/TimelineView'
import { SplitDivider } from './components/piano-roll/SplitDivider'
import { PianoRollPanel } from './components/piano-roll/PianoRollPanel'
import { MixerView } from './components/mixer/MixerView'
import { usePianoRollStore } from './stores/piano-roll-store'
import { useMixerStore } from './stores/mixer-store'
import './App.css'

export default function App() {
  const [engineStatus, setEngineStatus] = useState<string>('connecting...')
  const panelHeight = usePianoRollStore((s) => s.panelHeight)
  const activeClipId = usePianoRollStore((s) => s.activeClipId)
  const collapsed = panelHeight <= 36
  const showPianoRoll = activeClipId !== null
  const mixerVisible = useMixerStore((s) => s.mixerVisible)

  useEffect(() => {
    window.calliope.getEngineInfo()
      .then((info) => setEngineStatus(`JUCE ${info.juceVersion}`))
      .catch(() => setEngineStatus('offline'))
  }, [])

  const handleDividerDrag = useCallback((deltaY: number) => {
    const store = usePianoRollStore.getState()
    const maxH = window.innerHeight * 0.6
    const newH = Math.max(200, Math.min(maxH, store.panelHeight - deltaY))
    store.setPanelHeight(newH)
  }, [])

  const handleDividerDoubleClick = useCallback(() => {
    const store = usePianoRollStore.getState()
    store.setCollapsed(store.panelHeight > 36)
  }, [])

  return (
    <div className="h-screen w-screen bg-[#1a1a2e] text-[#eeeeee] overflow-hidden flex flex-col">
      <div className="flex-1 min-h-[200px] overflow-hidden">
        <TimelineView />
      </div>
      {showPianoRoll && (
        <>
          <SplitDivider onDrag={handleDividerDrag} onDoubleClick={handleDividerDoubleClick} />
          <div style={{ height: panelHeight, minHeight: collapsed ? 36 : 200 }} className="overflow-hidden">
            <PianoRollPanel />
          </div>
        </>
      )}
      {/* Mixer panel */}
      {mixerVisible && <MixerView />}
      {/* Engine status indicator */}
      <div className="absolute bottom-1 right-2 text-[10px] text-[#666666] pointer-events-none select-none">
        Engine: {engineStatus}
      </div>
    </div>
  )
}
