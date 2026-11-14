// IPC handlers that forward to the native audio engine.
// Channel names and shapes are defined by CalliopeAPI in ../shared/api.ts.

import { dialog, ipcMain } from 'electron'
import type { BrowserWindow } from 'electron'
import path from 'path'
import { native } from './native-bridge'
import type {
  EngineClipPayload,
  EngineCommand,
  ExportAudioParams,
  ExportFormat,
  ExportStemsParams,
} from '../shared/api'

const EXPORT_EXTENSIONS: Record<ExportFormat, string> = {
  wav16: 'wav',
  wav24: 'wav',
  mp3: 'mp3',
  flac: 'flac',
}

export function registerEngineIpc(getWindow: () => BrowserWindow | null): void {
  // Engine information
  ipcMain.handle('engine:getInfo', () => native.getEngineInfo())
  ipcMain.handle('engine:startTestTone', (_e, frequency: number) => native.startTestTone(frequency))
  ipcMain.handle('engine:stopTestTone', () => native.stopTestTone())

  // Lifecycle
  ipcMain.handle('engine:initialise', (_e, sampleRate: number, bufferSize: number) =>
    native.initialiseEngine(sampleRate, bufferSize),
  )
  ipcMain.handle('engine:shutdown', () => native.shutdownEngine())
  ipcMain.handle('engine:config:setBufferSize', (_e, bufferSize: number) =>
    native.setBufferSize(bufferSize),
  )

  // Transport
  ipcMain.handle('engine:transport:play', () => native.transportPlay())
  ipcMain.handle('engine:transport:stop', () => native.transportStop())
  ipcMain.handle('engine:transport:pause', () => native.transportPause())
  ipcMain.handle('engine:transport:setBpm', (_e, bpm: number) => native.setBpm(bpm))
  ipcMain.handle('engine:transport:setTimeSignature', (_e, numerator: number, denominator: number) =>
    native.setTimeSignature(numerator, denominator),
  )
  ipcMain.handle(
    'engine:transport:setLoop',
    (_e, startBeat: number, endBeat: number, enabled: boolean) =>
      native.setLoopRegion(startBeat, endBeat, enabled),
  )
  ipcMain.handle('engine:metronome:setEnabled', (_e, enabled: boolean) =>
    native.setMetronomeEnabled(enabled),
  )
  ipcMain.handle('engine:metronome:setVolume', (_e, volume: number) =>
    native.setMetronomeVolume(volume),
  )

  // State queries
  ipcMain.handle('engine:transport:getState', () => native.getTransportState())
  ipcMain.handle('engine:config:getAudioConfig', () => native.getAudioConfig())
  ipcMain.handle('engine:meter:getLevels', () => native.getMeterLevels())

  // Tracks and clips
  ipcMain.handle('engine:track:add', (_e, track: { trackId: string; instrument: string }) =>
    native.addTrack(track),
  )
  ipcMain.handle('engine:track:remove', (_e, trackId: string) => native.removeTrack(trackId))
  ipcMain.handle('engine:project:reset', () => native.resetProject())
  ipcMain.handle('engine:clip:update', (_e, clip: EngineClipPayload) => native.updateClip(clip))
  ipcMain.handle('engine:clip:remove', (_e, clipId: string) => native.removeClip(clipId))

  // Commands and parameters
  ipcMain.handle('command:dispatch', (_e, cmd: EngineCommand) => native.dispatchCommand(cmd))
  ipcMain.handle('command:describeParameters', () => native.describeParameters())

  // Export
  ipcMain.handle('export:getFormats', () => native.getExportFormats())

  ipcMain.handle('export:browsePath', async (_e, format: ExportFormat) => {
    const ext = EXPORT_EXTENSIONS[format] ?? 'wav'
    const window = getWindow()
    const options = {
      title: 'Export Audio',
      defaultPath: `export.${ext}`,
      filters: [{ name: `${ext.toUpperCase()} audio`, extensions: [ext] }],
    }
    const result = window
      ? await dialog.showSaveDialog(window, options)
      : await dialog.showSaveDialog(options)
    if (result.canceled || !result.filePath) return null
    approvedExportPaths.add(path.resolve(result.filePath))
    return result.filePath
  })

  ipcMain.handle('export:audio', async (event, params: ExportAudioParams) => {
    // Only write where the user said to, in the dialog above.
    if (!approvedExportPaths.has(path.resolve(params.outputPath))) return false
    return native.exportAudio(params, (fraction) => {
      if (!event.sender.isDestroyed()) event.sender.send('export:progress', fraction)
    })
  })

  ipcMain.handle('export:stems', async (event, params: ExportStemsParams) => {
    // Stems go in a folder beside an approved export file.
    const dir = path.resolve(params.outputDir)
    const allowed = [...approvedExportPaths].some((file) => path.dirname(file) === path.dirname(dir))
    if (!allowed) return false
    return native.exportStems({ ...params, outputDir: dir }, (fraction) => {
      if (!event.sender.isDestroyed()) event.sender.send('export:progress', fraction)
    })
  })
}

const approvedExportPaths = new Set<string>()
