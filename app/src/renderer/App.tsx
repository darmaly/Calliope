import { useState, useEffect, useCallback } from 'react'
import { TimelineView } from './components/timeline/TimelineView'
import { SplitDivider } from './components/piano-roll/SplitDivider'
import { PianoRollPanel } from './components/piano-roll/PianoRollPanel'
import { MixerPanel } from './components/mixer/MixerPanel'
import { ExportDialog } from './components/export/ExportDialog'
import { ExportProgress } from './components/export/ExportProgress'
import { Toast } from './components/shared/Toast'
import { usePianoRollStore } from './stores/piano-roll-store'
import { useMixerStore } from './stores/mixer-store'
import { useProjectStore } from './stores/project-store'
import './App.css'

export default function App() {
  const [engineStatus, setEngineStatus] = useState<string>('connecting...')
  const panelHeight = usePianoRollStore((s) => s.panelHeight)
  const activeClipId = usePianoRollStore((s) => s.activeClipId)
  const collapsed = panelHeight <= 36
  const showPianoRoll = activeClipId !== null
  const showMixer = useMixerStore((s) => s.mixerVisible)
  const mixerHeight = useMixerStore((s) => s.mixerHeight)
  const exportDialogOpen = useProjectStore((s) => s.exportDialogOpen)
  const exportProgress = useProjectStore((s) => s.exportProgress)
  const toastMessage = useProjectStore((s) => s.toastMessage)
  const hideToast = useProjectStore((s) => s.hideToast)
  const showExportDialog = useProjectStore((s) => s.showExportDialog)

  useEffect(() => {
    window.calliope.getEngineInfo()
      .then((info) => setEngineStatus(`JUCE ${info.juceVersion}`))
      .catch(() => setEngineStatus('offline'))
  }, [])

  // Listen for menu:export event from main process
  useEffect(() => {
    window.calliope.onShowExportDialog?.(() => {
      showExportDialog()
    })
    return () => {
      window.calliope.removeShowExportDialogListener?.()
    }
  }, [showExportDialog])

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

  const handleMixerDividerDrag = useCallback((deltaY: number) => {
    const store = useMixerStore.getState()
    const maxH = window.innerHeight * 0.6
    const newH = Math.max(240, Math.min(maxH, store.mixerHeight - deltaY))
    store.setMixerHeight(newH)
  }, [])

  const handleMixerDoubleClick = useCallback(() => {
    useMixerStore.getState().toggleMixerVisible()
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
      {showMixer && (
        <>
          <SplitDivider onDrag={handleMixerDividerDrag} onDoubleClick={handleMixerDoubleClick} />
          <div style={{ height: mixerHeight, minHeight: 240 }} className="overflow-hidden">
            <MixerPanel />
          </div>
        </>
      )}
      {/* Export dialog */}
      {exportDialogOpen && <ExportDialog />}
      {/* Export progress overlay */}
      {exportProgress !== null && <ExportProgress />}
      {/* Toast notification */}
      {toastMessage && (
        <Toast
          message={toastMessage.message}
          type={toastMessage.type}
          onRetry={toastMessage.retryFn}
          onDismiss={hideToast}
        />
      )}
      {/* Engine status indicator */}
      <div className="absolute bottom-1 right-2 text-[10px] text-[#666666] pointer-events-none select-none">
        Engine: {engineStatus}
      </div>
    </div>
  )
}
