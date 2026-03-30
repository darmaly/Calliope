import { app, BrowserWindow, ipcMain } from 'electron'
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
})

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit()
})

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) createWindow()
})
