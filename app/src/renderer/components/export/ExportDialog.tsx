import { useState, useEffect, useCallback } from 'react'
import { createPortal } from 'react-dom'
import { useProjectStore } from '../../stores/project-store'
import { useTimelineStore } from '../../stores/timeline-store'
import type { MidiNote } from '../../types/piano-roll'

type ExportFormat = 'wav16' | 'wav24' | 'mp3' | 'flac'

interface MidiEvent {
  beatPosition: number
  noteNumber: number
  velocity: number
  durationBeats: number
  trackId: string
}

function collectMidiEvents(): MidiEvent[] {
  const state = useTimelineStore.getState()
  const events: MidiEvent[] = []
  for (const track of state.tracks) {
    for (const clip of Object.values(state.clips).filter(
      (c) => c.trackId === track.id
    )) {
      if (clip.type === 'midi' && clip.notes) {
        for (const note of Object.values(clip.notes) as MidiNote[]) {
          events.push({
            beatPosition: clip.startBeat + note.startBeat,
            noteNumber: note.pitch,
            velocity: note.velocity,
            durationBeats: note.lengthBeats,
            trackId: track.id,
          })
        }
      }
    }
  }
  return events
}

function calculateTotalBeats(): number {
  const state = useTimelineStore.getState()
  let maxEnd = 0
  for (const clip of Object.values(state.clips)) {
    const end = clip.startBeat + clip.lengthBeats
    if (end > maxEnd) maxEnd = end
  }
  // Ensure at least 1 beat
  return Math.max(1, maxEnd)
}

const FORMAT_LABELS: Record<ExportFormat, string> = {
  wav16: 'WAV (16-bit)',
  wav24: 'WAV (24-bit)',
  mp3: 'MP3',
  flac: 'FLAC',
}

const FORMAT_EXTENSIONS: Record<ExportFormat, string> = {
  wav16: 'wav',
  wav24: 'wav',
  mp3: 'mp3',
  flac: 'flac',
}

const MP3_BITRATES = [128, 192, 256, 320] as const

export function ExportDialog() {
  const hideExportDialog = useProjectStore((s) => s.hideExportDialog)
  const setExportProgress = useProjectStore((s) => s.setExportProgress)
  const showToast = useProjectStore((s) => s.showToast)

  const [format, setFormat] = useState<ExportFormat>('wav24')
  const [mp3Bitrate, setMp3Bitrate] = useState<number>(192)
  const [exportStems, setExportStems] = useState(false)
  const [outputPath, setOutputPath] = useState<string>('')

  // Close on Escape
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        hideExportDialog()
      }
    }
    document.addEventListener('keydown', handleKeyDown)
    return () => document.removeEventListener('keydown', handleKeyDown)
  }, [hideExportDialog])

  const handleBrowse = useCallback(async () => {
    const path = await window.calliope.showExportPathDialog(format)
    if (path) {
      setOutputPath(path)
    }
  }, [format])

  const handleExport = useCallback(async () => {
    if (!outputPath) {
      showToast({ message: 'Please select an output file path.', type: 'error' })
      return
    }

    const midiEvents = collectMidiEvents()
    const totalBeats = calculateTotalBeats()
    const midiEventsJson = JSON.stringify(midiEvents)

    hideExportDialog()
    setExportProgress(0)

    const doExport = async () => {
      try {
        // Listen for progress updates
        window.calliope.onExportProgress((percent: number) => {
          setExportProgress(percent)
        })

        const success = await window.calliope.exportAudio({
          outputPath,
          format,
          mp3Bitrate: format === 'mp3' ? mp3Bitrate : 0,
          totalBeats,
          midiEventsJson,
        })

        if (exportStems) {
          // Derive stems directory from output path
          const stemsDir = outputPath.replace(
            /\.[^.]+$/,
            ''
          ) + '_stems'
          await window.calliope.exportStems({
            outputDir: stemsDir,
            totalBeats,
            midiEventsJson,
          })
        }

        window.calliope.removeExportProgressListener()
        setExportProgress(null)

        if (success) {
          showToast({ message: 'Export complete!', type: 'success' })
        } else {
          showToast({
            message: 'Export failed. Please try again.',
            type: 'error',
            retryFn: doExport,
          })
        }
      } catch (err) {
        window.calliope.removeExportProgressListener()
        setExportProgress(null)
        showToast({
          message: `Export error: ${err instanceof Error ? err.message : String(err)}`,
          type: 'error',
          retryFn: doExport,
        })
      }
    }

    await doExport()
  }, [outputPath, format, mp3Bitrate, exportStems, hideExportDialog, setExportProgress, showToast])

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
        <label className="block text-sm text-[#aaaacc] mb-1">Format</label>
        <select
          value={format}
          onChange={(e) => setFormat(e.target.value as ExportFormat)}
          className="w-full mb-3 px-3 py-2 rounded bg-[#16162b] border border-[#333366] text-[#eeeeee] text-sm outline-none focus:border-[#6666aa]"
        >
          {(Object.keys(FORMAT_LABELS) as ExportFormat[]).map((f) => (
            <option key={f} value={f}>
              {FORMAT_LABELS[f]}
            </option>
          ))}
        </select>

        {/* MP3 Bitrate (only shown for MP3) */}
        {format === 'mp3' && (
          <>
            <label className="block text-sm text-[#aaaacc] mb-1">Bitrate</label>
            <select
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
          Export individual stems (per-track WAV files)
        </label>

        {/* Output path */}
        <label className="block text-sm text-[#aaaacc] mb-1">Output Path</label>
        <div className="flex gap-2 mb-5">
          <input
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
    document.body
  )
}
