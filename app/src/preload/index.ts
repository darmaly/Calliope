import { contextBridge, ipcRenderer } from 'electron'
import type { IpcRendererEvent } from 'electron'
import type { CalliopeAPI, MenuAction, Unsubscribe } from '../shared/api'

/** Subscribe to a channel and return a function that removes exactly this listener. */
function listen<T>(channel: string, callback: (payload: T) => void): Unsubscribe {
  const handler = (_event: IpcRendererEvent, payload: T) => callback(payload)
  ipcRenderer.on(channel, handler)
  return () => {
    ipcRenderer.removeListener(channel, handler)
  }
}

const api: CalliopeAPI = {
  // Engine information
  getEngineInfo: () => ipcRenderer.invoke('engine:getInfo'),
  startTestTone: (frequency) => ipcRenderer.invoke('engine:startTestTone', frequency),
  stopTestTone: () => ipcRenderer.invoke('engine:stopTestTone'),

  // Lifecycle
  initialiseEngine: (sampleRate, bufferSize) =>
    ipcRenderer.invoke('engine:initialise', sampleRate, bufferSize),
  shutdownEngine: () => ipcRenderer.invoke('engine:shutdown'),
  setBufferSize: (bufferSize) => ipcRenderer.invoke('engine:config:setBufferSize', bufferSize),

  // Transport
  transportPlay: () => ipcRenderer.invoke('engine:transport:play'),
  transportStop: () => ipcRenderer.invoke('engine:transport:stop'),
  transportPause: () => ipcRenderer.invoke('engine:transport:pause'),
  setBpm: (bpm) => ipcRenderer.invoke('engine:transport:setBpm', bpm),
  setTimeSignature: (numerator, denominator) =>
    ipcRenderer.invoke('engine:transport:setTimeSignature', numerator, denominator),
  setLoopRegion: (startBeat, endBeat, enabled) =>
    ipcRenderer.invoke('engine:transport:setLoop', startBeat, endBeat, enabled),
  setMetronomeEnabled: (enabled) => ipcRenderer.invoke('engine:metronome:setEnabled', enabled),
  setMetronomeVolume: (volume) => ipcRenderer.invoke('engine:metronome:setVolume', volume),

  // State queries
  getTransportState: () => ipcRenderer.invoke('engine:transport:getState'),
  getAudioConfig: () => ipcRenderer.invoke('engine:config:getAudioConfig'),
  getMeterLevels: () => ipcRenderer.invoke('engine:meter:getLevels'),

  // Tracks and clips
  addTrack: (track) => ipcRenderer.invoke('engine:track:add', track),
  removeTrack: (trackId) => ipcRenderer.invoke('engine:track:remove', trackId),
  resetProject: () => ipcRenderer.invoke('engine:project:reset'),
  clipUpdate: (clip) => ipcRenderer.invoke('engine:clip:update', clip),
  clipRemove: (clipId) => ipcRenderer.invoke('engine:clip:remove', clipId),

  // Commands and parameters
  dispatchCommand: (cmd) => ipcRenderer.invoke('command:dispatch', cmd),
  describeParameters: () => ipcRenderer.invoke('command:describeParameters'),
  previewNoteOn: (trackId, note, velocity) =>
    ipcRenderer.invoke('command:dispatch', {
      command: 'instrument.noteOn',
      params: { trackId, note, velocity },
    }),
  previewNoteOff: (trackId, note) =>
    ipcRenderer.invoke('command:dispatch', {
      command: 'instrument.noteOff',
      params: { trackId, note },
    }),

  // Project files
  projectSave: (request) => ipcRenderer.invoke('project:save', request),
  projectOpen: () => ipcRenderer.invoke('project:open'),
  projectSetDirty: (dirty) => ipcRenderer.send('project:setDirty', dirty),

  // Export
  getExportFormats: () => ipcRenderer.invoke('export:getFormats'),
  showExportPathDialog: (format) => ipcRenderer.invoke('export:browsePath', format),
  exportAudio: (params) => ipcRenderer.invoke('export:audio', params),
  exportStems: (params) => ipcRenderer.invoke('export:stems', params),
  onExportProgress: (callback) => listen<number>('export:progress', callback),

  // Application menu
  onMenuAction: (callback) => listen<MenuAction>('menu:action', callback),
}

contextBridge.exposeInMainWorld('calliope', api)
