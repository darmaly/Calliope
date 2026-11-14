import { useState, useEffect, useCallback } from 'react'
import { TimelineView } from './components/timeline/TimelineView'
import { TransportBar } from './components/transport/TransportBar'
import { SplitDivider } from './components/shared/SplitDivider'
import { PianoRollPanel } from './components/piano-roll/PianoRollPanel'
import { MixerPanel } from './components/mixer/MixerPanel'
import { ExportDialog } from './components/export/ExportDialog'
import { ExportProgress } from './components/export/ExportProgress'
import { Toast } from './components/shared/Toast'
import { usePianoRollStore } from './stores/piano-roll-store'
import { useMixerStore } from './stores/mixer-store'
import { useProjectStore } from './stores/project-store'
import { newProject, openProject, redo, saveProject, startProjectSession, undo } from './project/session'
import { useAppStore } from './stores/app-store'
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
  const focusedPanel = useAppStore((s) => s.focusedPanel)

  // Window title effect
  const projectName = useProjectStore((s) => s.projectName)
  const isDirty = useProjectStore((s) => s.isDirty)
  useEffect(() => {
    document.title = `${isDirty ? '* ' : ''}${projectName} - Calliope`
  }, [projectName, isDirty])

  // Bring the audio engine up, then start the project session, which rebuilds
  // the engine from the project document and keeps the two in step.
  useEffect(() => {
    let cancelled = false
    let stopSession: (() => void) | undefined

    const start = async () => {
      try {
        // Serialised: querying devices and opening one at the same time races inside JUCE.
        const info = await window.calliope.getEngineInfo()
        const ok = await window.calliope.initialiseEngine(44100, 512)
        if (cancelled) return
        if (!ok) {
          setEngineStatus('no audio output device')
          return
        }
        const config = await window.calliope.getAudioConfig()
        const stop = await startProjectSession()
        if (cancelled) {
          stop()
          return
        }
        stopSession = stop
        setEngineStatus(`${info.juceVersion} - ${Math.round(config.sampleRate / 100) / 10} kHz`)
      } catch {
        if (!cancelled) setEngineStatus('offline')
      }
    }
    void start()

    return () => {
      cancelled = true
      stopSession?.()
    }
  }, [])

  // Application menu
  useEffect(() => {
    return window.calliope.onMenuAction((action) => {
      switch (action) {
        case 'new':
          void newProject()
          break
        case 'open':
          void openProject()
          break
        case 'save':
          void saveProject()
          break
        case 'save-as':
          void saveProject({ saveAs: true })
          break
        case 'export':
          showExportDialog()
          break
        case 'undo':
          undo()
          break
        case 'redo':
          redo()
          break
      }
    })
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

  const focusBorder = (panel: 'timeline' | 'piano-roll' | 'mixer') =>
    focusedPanel === panel ? 'border-l-2 border-[#6c63ff]' : 'border-l-2 border-transparent'

  return (
    <div className="h-screen w-screen bg-[#1a1a2e] text-[#eeeeee] overflow-hidden flex flex-col">
      <TransportBar />
      <div
        className={`flex-1 min-h-[200px] overflow-hidden ${focusBorder('timeline')}`}
        onPointerDown={() => useAppStore.getState().setFocusedPanel('timeline')}
      >
        <TimelineView />
      </div>
      {showPianoRoll && (
        <>
          <SplitDivider onDrag={handleDividerDrag} onDoubleClick={handleDividerDoubleClick} />
          <div
            style={{ height: panelHeight, minHeight: collapsed ? 36 : 200 }}
            className={`overflow-hidden ${focusBorder('piano-roll')}`}
            onPointerDown={() => useAppStore.getState().setFocusedPanel('piano-roll')}
          >
            <PianoRollPanel />
          </div>
        </>
      )}
      {showMixer && (
        <>
          <SplitDivider onDrag={handleMixerDividerDrag} onDoubleClick={handleMixerDoubleClick} />
          <div
            style={{ height: mixerHeight, minHeight: 240 }}
            className={`overflow-hidden ${focusBorder('mixer')}`}
            onPointerDown={() => useAppStore.getState().setFocusedPanel('mixer')}
          >
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
