import { useState, useEffect, useCallback } from 'react'
import { createPortal } from 'react-dom'
import { useProjectStore } from '../../stores/project-store'
import { useTimelineStore } from '../../stores/timeline-store'
import { engineSettled } from '../../project/session'
import type { ExportFormat, ExportFormatSupport } from '../../../shared/api'

const FORMAT_LABELS: Record<ExportFormat, string> = {
  wav24: 'WAV (24-bit)',
  wav16: 'WAV (16-bit)',
  flac: 'FLAC',
  mp3: 'MP3',
}

const MP3_BITRATES = [128, 192, 256, 320] as const

/** True when there is at least one note to render. */
function projectHasNotes(): boolean {
  const { clips } = useTimelineStore.getState()
  return Object.values(clips).some((c) => c.type === 'midi' && c.notes && Object.keys(c.notes).length > 0)
}

export function ExportDialog() {
  const hideExportDialog = useProjectStore((s) => s.hideExportDialog)
  const setExportProgress = useProjectStore((s) => s.setExportProgress)
  const showToast = useProjectStore((s) => s.showToast)

  const [format, setFormat] = useState<ExportFormat>('wav24')
  const [mp3Bitrate, setMp3Bitrate] = useState<number>(192)
  const [exportStems, setExportStems] = useState(false)
  const [outputPath, setOutputPath] = useState<string>('')
  const [supported, setSupported] = useState<ExportFormatSupport | null>(null)

  useEffect(() => {
    let cancelled = false
    window.calliope
      .getExportFormats()
      .then((formats) => {
        if (!cancelled) setSupported(formats)
      })
      .catch(() => {})
    return () => {
      cancelled = true
    }
  }, [])

  // Close on Escape
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape') hideExportDialog()
    }
    document.addEventListener('keydown', handleKeyDown)
    return () => document.removeEventListener('keydown', handleKeyDown)
  }, [hideExportDialog])

  const handleBrowse = useCallback(async () => {
    const path = await window.calliope.showExportPathDialog(format)
    if (path) setOutputPath(path)
  }, [format])

  const handleFormatChange = (next: ExportFormat) => {
    setFormat(next)
    // The chosen file name carries the old format's extension.
    setOutputPath('')
  }

  const handleExport = useCallback(async () => {
    if (!outputPath) {
      showToast({ message: 'Choose where to save the export first.', type: 'error' })
      return
    }
    if (!projectHasNotes()) {
      showToast({ message: 'There is nothing to export yet: the project has no notes.', type: 'error' })
      return
    }

    hideExportDialog()

    const doExport = async () => {
      setExportProgress(0)
      const stopListening = window.calliope.onExportProgress((fraction) => setExportProgress(fraction))
      try {
        // Stop playback and make sure the engine has every edit before rendering.
        await window.calliope.transportStop()
        await engineSettled()

        const mixOk = await window.calliope.exportAudio({
          outputPath,
          format,
          mp3Bitrate: format === 'mp3' ? mp3Bitrate : undefined,
        })

        let stemsOk = true
        if (mixOk && exportStems) {
          const { tracks } = useTimelineStore.getState()
          setExportProgress(0)
          stemsOk = await window.calliope.exportStems({
            outputDir: outputPath.replace(/\.[^./\\]+$/, '') + '_stems',
            trackNames: Object.fromEntries(tracks.map((t) => [t.id, t.name])),
          })
        }

        if (mixOk && stemsOk) {
          showToast({ message: exportStems ? 'Export complete: mix and stems.' : 'Export complete.', type: 'success' })
        } else {
          showToast({
            message: mixOk ? 'The mix was exported, but the stems failed.' : 'Export failed.',
            type: 'error',
            retryFn: doExport,
          })
        }
      } catch (err) {
        showToast({
          message: `Export error: ${err instanceof Error ? err.message : String(err)}`,
          type: 'error',
          retryFn: doExport,
        })
      } finally {
        stopListening()
        setExportProgress(null)
      }
    }

    await doExport()
  }, [outputPath, format, mp3Bitrate, exportStems, hideExportDialog, setExportProgress, showToast])

  const formats = Object.keys(FORMAT_LABELS) as ExportFormat[]

  return createPortal(
    <div
      className="fixed inset-0 z-50 flex items-center justify-center"
      style={{ backgroundColor: 'rgba(0, 0, 0, 0.6)' }}
      onPointerDown={(e) => {
        if (e.target === e.currentTarget) hideExportDialog()
      }}
    >
      <div className="bg-[#1e1e3a] rounded-lg shadow-2xl border border-[#333366] w-[420px] p-6">
        <h2 className="text-lg font-semibold text-[#eeeeee] mb-4">Export Audio</h2>

        {/* Format */}
        <label className="block text-sm text-[#aaaacc] mb-1" htmlFor="export-format">
          Format
        </label>
        <select
          id="export-format"
          value={format}
          onChange={(e) => handleFormatChange(e.target.value as ExportFormat)}
          className="w-full mb-3 px-3 py-2 rounded bg-[#16162b] border border-[#333366] text-[#eeeeee] text-sm outline-none focus:border-[#6666aa]"
        >
          {formats.map((f) => {
            const available = supported === null || supported[f]
            return (
              <option key={f} value={f} disabled={!available}>
                {FORMAT_LABELS[f]}
                {available ? '' : ' (not available in this build)'}
              </option>
            )
          })}
        </select>

        {/* MP3 Bitrate (only shown for MP3) */}
        {format === 'mp3' && (
          <>
            <label className="block text-sm text-[#aaaacc] mb-1" htmlFor="export-bitrate">
              Bitrate
            </label>
            <select
              id="export-bitrate"
              value={mp3Bitrate}
              onChange={(e) => setMp3Bitrate(Number(e.target.value))}
              className="w-full mb-3 px-3 py-2 rounded bg-[#16162b] border border-[#333366] text-[#eeeeee] text-sm outline-none focus:border-[#6666aa]"
            >
              {MP3_BITRATES.map((br) => (
                <option key={br} value={br}>
                  {br} kbps
                </option>
              ))}
            </select>
          </>
        )}

        {/* Stems checkbox */}
        <label className="flex items-center gap-2 mb-4 text-sm text-[#aaaacc] cursor-pointer select-none">
          <input
            type="checkbox"
            checked={exportStems}
            onChange={(e) => setExportStems(e.target.checked)}
            className="accent-[#6666aa]"
          />
          Also export each track on its own (24-bit WAV stems)
        </label>

        {/* Output path */}
        <label className="block text-sm text-[#aaaacc] mb-1" htmlFor="export-path">
          Save to
        </label>
        <div className="flex gap-2 mb-5">
          <input
            id="export-path"
            type="text"
            value={outputPath}
            readOnly
            placeholder="Choose export location..."
            className="flex-1 px-3 py-2 rounded bg-[#16162b] border border-[#333366] text-[#eeeeee] text-sm outline-none truncate"
          />
          <button
            onClick={handleBrowse}
            className="px-3 py-2 rounded bg-[#333366] text-[#eeeeee] text-sm hover:bg-[#444488] transition-colors"
          >
            Browse
          </button>
        </div>

        {/* Actions */}
        <div className="flex justify-end gap-3">
          <button
            onClick={hideExportDialog}
            className="px-4 py-2 rounded bg-[#2a2a44] text-[#aaaacc] text-sm hover:bg-[#333366] transition-colors"
          >
            Cancel
          </button>
          <button
            onClick={handleExport}
            disabled={!outputPath}
            className="px-4 py-2 rounded bg-[#5555aa] text-[#eeeeee] text-sm hover:bg-[#6666bb] transition-colors disabled:opacity-40 disabled:cursor-not-allowed"
          >
            Export
          </button>
        </div>
      </div>
    </div>,
    document.body,
  )
}
