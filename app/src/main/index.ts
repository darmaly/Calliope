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
  } else {
    mainWindow.loadFile(join(__dirname, '../renderer/index.html'))
  }

  mainWindow.on('closed', () => { mainWindow = null })
}

// IPC handlers -- bridge React UI to native addon
ipcMain.handle('engine:getInfo', async () => {
  return await native.getEngineInfo()
})

ipcMain.handle('engine:startTestTone', async (_event, frequency: number) => {
  return await native.startTestTone(frequency)
})

ipcMain.handle('engine:stopTestTone', async () => {
  return await native.stopTestTone()
})

app.whenReady().then(createWindow)

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit()
})

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) createWindow()
})
