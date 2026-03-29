import { app, BrowserWindow, ipcMain, dialog } from 'electron'
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

// Phase 9 — Project save/load
const PROJECT_FILTERS = [
  { name: 'LuneyTunes Project', extensions: ['ltproj'] },
  { name: 'All Files', extensions: ['*'] }
]

let currentProjectPath: string | null = null
let autosaveTimer: ReturnType<typeof setInterval> | null = null
let autosaveEnabled = true
let autosaveIntervalMs = 120000
let projectDirty = false

ipcMain.handle('project:save', async (_event, filePath?: string) => {
  const savePath = filePath ?? currentProjectPath
  if (!savePath) {
    // No path yet — trigger Save As
    const win = BrowserWindow.getFocusedWindow()
    if (!win) return { success: false, filePath: null }
    const result = await dialog.showSaveDialog(win, {
      title: 'Save Project',
      filters: PROJECT_FILTERS,
      defaultPath: 'Untitled.ltproj'
    })
    if (result.canceled || !result.filePath) return { success: false, filePath: null }
    currentProjectPath = result.filePath
    const success = await native.saveProject(currentProjectPath)
    if (success) projectDirty = false
    return { success, filePath: currentProjectPath }
  }
  const success = await native.saveProject(savePath)
  if (success) {
    currentProjectPath = savePath
    projectDirty = false
  }
  return { success, filePath: savePath }
})

ipcMain.handle('project:saveAs', async () => {
  const win = BrowserWindow.getFocusedWindow()
  if (!win) return { success: false, filePath: null }
  const result = await dialog.showSaveDialog(win, {
    title: 'Save Project As',
    filters: PROJECT_FILTERS,
    defaultPath: currentProjectPath ?? 'Untitled.ltproj'
  })
  if (result.canceled || !result.filePath) return { success: false, filePath: null }
  currentProjectPath = result.filePath
  const success = await native.saveProject(currentProjectPath)
  if (success) projectDirty = false
  return { success, filePath: currentProjectPath }
})

ipcMain.handle('project:load', async () => {
  const win = BrowserWindow.getFocusedWindow()
  if (!win) return { success: false, filePath: null }
  const result = await dialog.showOpenDialog(win, {
    title: 'Open Project',
    filters: PROJECT_FILTERS,
    properties: ['openFile']
  })
  if (result.canceled || result.filePaths.length === 0) return { success: false, filePath: null }
  const loadPath = result.filePaths[0]
  const success = await native.loadProject(loadPath)
  if (success) {
    currentProjectPath = loadPath
    projectDirty = false
  }
  return { success, filePath: loadPath }
})

ipcMain.handle('project:new', async () => {
  currentProjectPath = null
  projectDirty = false
  return { success: true }
})

ipcMain.handle('project:getInfo', async () => {
  return {
    filePath: currentProjectPath,
    isDirty: projectDirty
  }
})

ipcMain.handle('project:setAutosave', async (_event, enabled: boolean, intervalMs?: number) => {
  autosaveEnabled = enabled
  if (intervalMs !== undefined) autosaveIntervalMs = intervalMs
  restartAutosaveTimer()
  return { autosaveEnabled, autosaveIntervalMs }
})

ipcMain.handle('project:getAutosaveConfig', async () => {
  return { autosaveEnabled, autosaveIntervalMs }
})

ipcMain.handle('project:markDirty', async () => {
  projectDirty = true
})

// Phase 9 — Export
ipcMain.handle(
  'project:export',
  async (_event, params: {
    outputPath: string
    format: string
    mp3Bitrate: number
    totalBeats: number
    midiEventsJson: string
  }) => {
    return await native.exportAudio(
      params.outputPath,
      params.format,
      params.mp3Bitrate,
      params.totalBeats,
      params.midiEventsJson,
      (percent: number) => {
        const windows = BrowserWindow.getAllWindows()
        for (const win of windows) {
          win.webContents.send('project:exportProgress', percent)
        }
      }
    )
  }
)

ipcMain.handle(
  'project:exportStems',
  async (_event, params: {
    outputDir: string
    totalBeats: number
    midiEventsJson: string
  }) => {
    return await native.exportStems(
      params.outputDir,
      params.totalBeats,
      params.midiEventsJson,
      (percent: number) => {
        const windows = BrowserWindow.getAllWindows()
        for (const win of windows) {
          win.webContents.send('project:exportProgress', percent)
        }
      }
    )
  }
)

ipcMain.handle(
  'project:loadState',
  async (_event, jsonString: string) => {
    return await native.loadProjectState(jsonString)
  }
)

// Phase 9 — Browse export path dialog
const EXPORT_FILTERS: Record<string, Electron.FileFilter[]> = {
  wav16: [{ name: 'WAV Audio (16-bit)', extensions: ['wav'] }],
  wav24: [{ name: 'WAV Audio (24-bit)', extensions: ['wav'] }],
  mp3: [{ name: 'MP3 Audio', extensions: ['mp3'] }],
  flac: [{ name: 'FLAC Audio', extensions: ['flac'] }],
}

ipcMain.handle('project:browseExportPath', async (_event, format: string) => {
  const win = BrowserWindow.getFocusedWindow()
  if (!win) return null
  const filters = EXPORT_FILTERS[format] ?? [{ name: 'Audio Files', extensions: ['wav', 'mp3', 'flac'] }]
  const result = await dialog.showSaveDialog(win, {
    title: 'Export Audio',
    filters,
    defaultPath: `export.${filters[0]?.extensions[0] ?? 'wav'}`,
  })
  if (result.canceled || !result.filePath) return null
  return result.filePath
})

function restartAutosaveTimer(): void {
  if (autosaveTimer) {
    clearInterval(autosaveTimer)
    autosaveTimer = null
  }
  if (autosaveEnabled) {
    autosaveTimer = setInterval(async () => {
      if (projectDirty && currentProjectPath) {
        const success = await native.saveProject(currentProjectPath)
        if (success) {
          projectDirty = false
          // Notify renderer of autosave
          const windows = BrowserWindow.getAllWindows()
          for (const win of windows) {
            win.webContents.send('project:autosaved', {
              filePath: currentProjectPath,
              timestamp: new Date().toISOString()
            })
          }
        }
      }
    }, autosaveIntervalMs)
  }
}

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
  restartAutosaveTimer()
})

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit()
})

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) createWindow()
})
