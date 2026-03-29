import { useEffect, useRef } from 'react'
import { createPortal } from 'react-dom'
import { useProjectStore } from '../../stores/project-store'

export function ExportProgress() {
  const exportProgress = useProjectStore((s) => s.exportProgress)
  const setExportProgress = useProjectStore((s) => s.setExportProgress)
  const dismissTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null)

  const percent = exportProgress ?? 0
  const isComplete = percent >= 1

  // Auto-dismiss after completion
  useEffect(() => {
    if (isComplete) {
      dismissTimerRef.current = setTimeout(() => {
        setExportProgress(null)
      }, 2000)
    }
    return () => {
      if (dismissTimerRef.current) {
        clearTimeout(dismissTimerRef.current)
      }
    }
  }, [isComplete, setExportProgress])

  const displayPercent = Math.round(percent * 100)

  return createPortal(
    <div
      className="fixed inset-0 z-50 flex items-center justify-center"
      style={{ backgroundColor: 'rgba(0, 0, 0, 0.5)' }}
    >
      <div className="bg-[#1e1e3a] rounded-lg shadow-2xl border border-[#333366] w-[340px] p-6">
        <p className="text-sm text-[#eeeeee] mb-3 text-center">
          {isComplete ? 'Export complete!' : `Exporting... ${displayPercent}%`}
        </p>
        <div className="w-full h-3 rounded-full bg-[#16162b] overflow-hidden">
          <div
            className="h-full rounded-full transition-all duration-200"
            style={{
              width: `${displayPercent}%`,
              backgroundColor: isComplete ? '#44bb66' : '#5555aa',
            }}
          />
        </div>
      </div>
    </div>,
    document.body
  )
}
