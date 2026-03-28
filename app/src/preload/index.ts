import { contextBridge, ipcRenderer } from 'electron'

contextBridge.exposeInMainWorld('calliope', {
  getEngineInfo: () => ipcRenderer.invoke('engine:getInfo'),
  startTestTone: (frequency: number) => ipcRenderer.invoke('engine:startTestTone', frequency),
  stopTestTone: () => ipcRenderer.invoke('engine:stopTestTone'),
})
