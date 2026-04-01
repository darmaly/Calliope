import { app, BrowserWindow, dialog, ipcMain } from 'electron'
import { join } from 'path'
import { native } from './native-bridge'

let mainWindow: BrowserWindow | null = null

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1200,
    height: 800,
    webPreferences: {
      preload: join(__dirname, '../preload/index.js'),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: false,
    },
  })

  // In dev, load from vite dev server; in prod, load from file
  if (process.env.ELECTRON_RENDERER_URL) {
    mainWindow.loadURL(process.env.ELECTRON_RENDERER_URL)
    mainWindow.webContents.openDevTools()
  } else {
    mainWindow.loadFile(join(__dirname, '../renderer/index.html'))
  }

  mainWindow.on('closed', () => { mainWindow = null })
}

// IPC handlers -- bridge React UI to native addon

// Phase 1
ipcMain.handle('engine:getInfo', async () => {
  return await native.getEngineInfo()
})

ipcMain.handle('engine:startTestTone', async (_event, frequency: number) => {
  return await native.startTestTone(frequency)
})

ipcMain.handle('engine:stopTestTone', async () => {
  return await native.stopTestTone()
})

// Phase 2 — Engine lifecycle
ipcMain.handle('engine:initialise', async (_event, sampleRate: number, bufferSize: number) => {
  return await native.initialiseEngine(sampleRate, bufferSize)
})

ipcMain.handle('engine:shutdown', async () => {
  return await native.shutdownEngine()
})

// Phase 2 — Transport
ipcMain.handle('engine:transport:play', async () => {
  return await native.transportPlay()
})

ipcMain.handle('engine:transport:stop', async () => {
  return await native.transportStop()
})

ipcMain.handle('engine:transport:pause', async () => {
  return await native.transportPause()
})

ipcMain.handle('engine:transport:setBpm', async (_event, bpm: number) => {
  return await native.setBpm(bpm)
})

ipcMain.handle('engine:transport:setTimeSignature', async (_event, num: number, den: number) => {
  return await native.setTimeSignature(num, den)
})

ipcMain.handle('engine:transport:setLoop', async (_event, startBeat: number, endBeat: number, enabled: boolean) => {
  return await native.setLoopRegion(startBeat, endBeat, enabled)
})

// Phase 2 — Config
ipcMain.handle('engine:config:setBufferSize', async (_event, bufferSize: number) => {
  return await native.setBufferSize(bufferSize)
})

// Phase 2 — Metronome
ipcMain.handle('engine:metronome:setEnabled', async (_event, enabled: boolean) => {
  return await native.setMetronomeEnabled(enabled)
})

ipcMain.handle('engine:metronome:setVolume', async (_event, volume: number) => {
  return await native.setMetronomeVolume(volume)
})

// Phase 2 — State queries
ipcMain.handle('engine:transport:getState', async () => {
  return await native.getTransportState()
})

ipcMain.handle('engine:config:getAudioConfig', async () => {
  return await native.getAudioConfig()
})

// Phase 3 — Command dispatch (Phase 4 instrument + Phase 5 effect commands flow through command:dispatch)
// Supported commands: instrument.noteOn, instrument.noteOff, drumMachine.loadSample,
//   effect.insert, effect.remove, effect.reorder, effect.bypass
ipcMain.handle(
  'command:dispatch',
  async (_event, cmd: { command: string; params: Record<string, unknown> }) => {
    return await native.dispatchCommand(cmd)
  }
)

ipcMain.handle('command:undo', async () => {
  return await native.commandUndo()
})

ipcMain.handle('command:redo', async () => {
  return await native.commandRedo()
})

ipcMain.handle('command:getState', async () => {
  return await native.getProjectState()
})

ipcMain.handle('command:getParameterIds', async () => {
  return await native.getParameterIds()
})

// Phase 10.1 — Clip operations
ipcMain.handle('engine:clip:add', async (_event, clip) => {
  return await native.addClip(clip)
})

ipcMain.handle('engine:clip:remove', async (_event, clipId: string) => {
  return await native.removeClip(clipId)
})

ipcMain.handle('engine:clip:update', async (_event, clip) => {
  return await native.updateClip(clip)
})

ipcMain.handle('engine:clip:clear', async () => {
  return await native.clearClips()
})

// Phase 8 — Metering
ipcMain.handle('engine:meter:getLevels', async () => {
  return await native.getMeterLevels()
})

// Phase 9 — Project save/load

// Track current project file path and autosave state in main process
let currentProjectPath: string | null = null
let projectDirty = false
let autosaveEnabled = true
let autosaveIntervalMs = 120000
let autosaveTimer: ReturnType<typeof setInterval> | null = null

function startAutosaveTimer() {
  stopAutosaveTimer()
  if (!autosaveEnabled) return
  autosaveTimer = setInterval(async () => {
    if (!projectDirty || !currentProjectPath) return
    try {
      const success = await native.saveProject(currentProjectPath)
      if (success) {
        projectDirty = false
        const windows = BrowserWindow.getAllWindows()
        for (const win of windows) {
          win.webContents.send('project:autosaved', {
            filePath: currentProjectPath,
            timestamp: new Date().toISOString(),
          })
        }
      }
    } catch {
      // Autosave failed silently — user can still manual-save
    }
  }, autosaveIntervalMs)
}

function stopAutosaveTimer() {
  if (autosaveTimer !== null) {
    clearInterval(autosaveTimer)
    autosaveTimer = null
  }
}

ipcMain.handle('project:save', async (_event, filePath?: string) => {
  let savePath = filePath ?? currentProjectPath
  if (!savePath) {
    // No path yet — show Save As dialog
    const result = await dialog.showSaveDialog({
      title: 'Save Project',
      defaultPath: 'Untitled.ltproj',
      filters: [{ name: 'LuneyTunes Project', extensions: ['ltproj'] }],
    })
    if (result.canceled || !result.filePath) {
      return { success: false, filePath: null }
    }
    savePath = result.filePath
  }
  const success = await native.saveProject(savePath)
  if (success) {
    currentProjectPath = savePath
    projectDirty = false
  }
  return { success, filePath: savePath }
})

ipcMain.handle('project:saveAs', async () => {
  const result = await dialog.showSaveDialog({
    title: 'Save Project As',
    defaultPath: currentProjectPath ?? 'Untitled.ltproj',
    filters: [{ name: 'LuneyTunes Project', extensions: ['ltproj'] }],
  })
  if (result.canceled || !result.filePath) {
    return { success: false, filePath: null }
  }
  const success = await native.saveProject(result.filePath)
  if (success) {
    currentProjectPath = result.filePath
    projectDirty = false
  }
  return { success, filePath: result.filePath }
})

ipcMain.handle('project:load', async () => {
  const result = await dialog.showOpenDialog({
    title: 'Open Project',
    filters: [{ name: 'LuneyTunes Project', extensions: ['ltproj'] }],
    properties: ['openFile'],
  })
  if (result.canceled || result.filePaths.length === 0) {
    return { success: false, filePath: null }
  }
  const filePath = result.filePaths[0]
  const success = await native.loadProject(filePath)
  if (success) {
    currentProjectPath = filePath
    projectDirty = false
  }
  return { success, filePath }
})

ipcMain.handle('project:new', async () => {
  // Reset engine to default state by loading an empty state
  currentProjectPath = null
  projectDirty = false
  return true
})

ipcMain.handle('project:getInfo', async () => {
  return {
    filePath: currentProjectPath,
    isDirty: projectDirty,
    autosaveEnabled,
    autosaveIntervalMs,
  }
})

ipcMain.handle('project:setAutosave', async (_event, enabled: boolean, intervalMs?: number) => {
  autosaveEnabled = enabled
  if (intervalMs !== undefined) autosaveIntervalMs = intervalMs
  if (enabled) {
    startAutosaveTimer()
  } else {
    stopAutosaveTimer()
  }
  return true
})

ipcMain.handle('project:getAutosaveConfig', async () => {
  return {
    enabled: autosaveEnabled,
    intervalMs: autosaveIntervalMs,
  }
})

ipcMain.handle('project:markDirty', async () => {
  projectDirty = true
  return true
})

// Phase 9 — Export

ipcMain.handle('project:export', async (event, params: {
  outputPath: string; format: string; mp3Bitrate: number;
  totalBeats: number; midiEventsJson: string;
}) => {
  // Note: The native exportAudio accepts positional args. Progress callback
  // would be a 6th arg, but we forward progress via IPC events instead.
  // For now, call without progress callback — the renderer polls or we can
  // enhance this later if the native addon supports a JS-callable progress cb.
  const result = await native.exportAudio(
    params.outputPath, params.format, params.mp3Bitrate,
    params.totalBeats, params.midiEventsJson
  )
  // Send 100% completion
  const sender = event.sender
  sender.send('project:exportProgress', 1.0)
  return result
})

ipcMain.handle('project:exportStems', async (_event, params: {
  outputDir: string; totalBeats: number; midiEventsJson: string;
}) => {
  return await native.exportStems(params.outputDir, params.totalBeats, params.midiEventsJson)
})

ipcMain.handle('project:loadState', async (_event, json: string) => {
  return await native.loadProjectState(json)
})

ipcMain.handle('project:browseExportPath', async (_event, format: string) => {
  const extMap: Record<string, string> = {
    wav16: 'wav', wav24: 'wav', mp3: 'mp3', flac: 'flac'
  }
  const ext = extMap[format] ?? 'wav'
  const result = await dialog.showSaveDialog({
    title: 'Export Audio',
    defaultPath: `export.${ext}`,
    filters: [{ name: `Audio (${ext.toUpperCase()})`, extensions: [ext] }],
  })
  if (result.canceled || !result.filePath) return null
  return result.filePath
})

// Subscribe to engine events and forward to renderer windows
let eventSubscribed = false
function subscribeToEngineEvents(): void {
  if (eventSubscribed) return
  native.subscribeToEvents((event) => {
    const windows = BrowserWindow.getAllWindows()
    for (const win of windows) {
      win.webContents.send('command:event', event)
    }
  })
  eventSubscribed = true
}

app.whenReady().then(() => {
  createWindow()
  subscribeToEngineEvents()
  startAutosaveTimer()
})

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit()
})

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) createWindow()
})
