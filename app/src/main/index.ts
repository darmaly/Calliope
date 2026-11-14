import { app, BrowserWindow, Menu, dialog, shell } from 'electron'
import type { MenuItemConstructorOptions } from 'electron'
import { join } from 'path'
import { registerEngineIpc } from './engine-ipc'
import { projectHasUnsavedChanges, registerProjectIpc } from './project-files'
import type { MenuAction } from '../shared/api'

let mainWindow: BrowserWindow | null = null

function createWindow(): void {
  const window = new BrowserWindow({
    width: 1280,
    height: 820,
    minWidth: 900,
    minHeight: 600,
    backgroundColor: '#1a1a2e',
    webPreferences: {
      preload: join(__dirname, '../preload/index.js'),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true,
    },
  })
  mainWindow = window

  // The app is a single local page. It never opens windows or navigates away;
  // links (for example in an assistant reply) open in the user's browser.
  window.webContents.setWindowOpenHandler(({ url }) => {
    if (/^https?:\/\//i.test(url)) void shell.openExternal(url)
    return { action: 'deny' }
  })
  window.webContents.on('will-navigate', (event, url) => {
    if (url !== window.webContents.getURL()) event.preventDefault()
  })

  // In dev, load from vite dev server; in prod, load from file
  if (process.env.ELECTRON_RENDERER_URL) {
    void window.loadURL(process.env.ELECTRON_RENDERER_URL)
    if (!process.env.CALLIOPE_NO_DEVTOOLS) window.webContents.openDevTools()
  } else {
    void window.loadFile(join(__dirname, '../renderer/index.html'))
  }

  window.on('close', (event) => {
    if (!projectHasUnsavedChanges()) return
    const choice = dialog.showMessageBoxSync(window, {
      type: 'warning',
      buttons: ['Cancel', 'Close Without Saving'],
      defaultId: 0,
      cancelId: 0,
      title: 'Unsaved changes',
      message: 'This project has unsaved changes.',
      detail: 'If you close now, those changes will be lost.',
    })
    if (choice === 0) event.preventDefault()
  })

  window.on('closed', () => {
    if (mainWindow === window) mainWindow = null
  })
}

function sendMenuAction(action: MenuAction): void {
  mainWindow?.webContents.send('menu:action', action)
}

function buildMenu(): void {
  // The renderer handles these shortcuts itself (so they can depend on which
  // panel has focus), so the menu only displays them: registering them here
  // as well would swallow the key before the page sees it.
  const item = (label: string, action: MenuAction, accelerator?: string): MenuItemConstructorOptions => ({
    label,
    accelerator,
    registerAccelerator: false,
    click: () => sendMenuAction(action),
  })

  const template: MenuItemConstructorOptions[] = [
    ...(process.platform === 'darwin' ? [{ role: 'appMenu' as const }] : []),
    {
      label: 'File',
      submenu: [
        item('New Project', 'new', 'CmdOrCtrl+N'),
        item('Open Project...', 'open', 'CmdOrCtrl+O'),
        { type: 'separator' },
        item('Save', 'save', 'CmdOrCtrl+S'),
        item('Save As...', 'save-as', 'CmdOrCtrl+Shift+S'),
        { type: 'separator' },
        item('Export Audio...', 'export', 'CmdOrCtrl+Shift+E'),
        { type: 'separator' },
        process.platform === 'darwin' ? { role: 'close' } : { role: 'quit' },
      ],
    },
    {
      label: 'Edit',
      submenu: [
        item('Undo', 'undo', 'CmdOrCtrl+Z'),
        item('Redo', 'redo', 'CmdOrCtrl+Shift+Z'),
        { type: 'separator' },
        { role: 'cut', registerAccelerator: false },
        { role: 'copy', registerAccelerator: false },
        { role: 'paste', registerAccelerator: false },
      ],
    },
    {
      label: 'Assistant',
      submenu: [
        item('Show or Hide Assistant', 'toggle-assistant', 'CmdOrCtrl+J'),
        item('AI Provider Settings...', 'ai-settings'),
      ],
    },
    {
      label: 'View',
      submenu: [
        { role: 'togglefullscreen' },
        { role: 'toggleDevTools' },
        ...(app.isPackaged ? [] : [{ role: 'reload' as const }]),
      ],
    },
  ]

  Menu.setApplicationMenu(Menu.buildFromTemplate(template))
}

registerEngineIpc(() => mainWindow)
registerProjectIpc(() => mainWindow)

void app.whenReady().then(() => {
  buildMenu()
  createWindow()
})

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit()
})

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) createWindow()
})
