import { contextBridge, ipcRenderer } from 'electron'

contextBridge.exposeInMainWorld('calliope', {
  // Phase 1
  getEngineInfo: () => ipcRenderer.invoke('engine:getInfo'),
  startTestTone: (frequency: number) => ipcRenderer.invoke('engine:startTestTone', frequency),
  stopTestTone: () => ipcRenderer.invoke('engine:stopTestTone'),

  // Phase 2 — Engine lifecycle
  initialiseEngine: (sampleRate: number, bufferSize: number) =>
    ipcRenderer.invoke('engine:initialise', sampleRate, bufferSize),
  shutdownEngine: () => ipcRenderer.invoke('engine:shutdown'),

  // Phase 2 — Transport
  transportPlay: () => ipcRenderer.invoke('engine:transport:play'),
  transportStop: () => ipcRenderer.invoke('engine:transport:stop'),
  transportPause: () => ipcRenderer.invoke('engine:transport:pause'),
  setBpm: (bpm: number) => ipcRenderer.invoke('engine:transport:setBpm', bpm),
  setTimeSignature: (num: number, den: number) =>
    ipcRenderer.invoke('engine:transport:setTimeSignature', num, den),
  setLoopRegion: (startBeat: number, endBeat: number, enabled: boolean) =>
    ipcRenderer.invoke('engine:transport:setLoop', startBeat, endBeat, enabled),

  // Phase 2 — Config
  setBufferSize: (bufferSize: number) =>
    ipcRenderer.invoke('engine:config:setBufferSize', bufferSize),

  // Phase 2 — Metronome
  setMetronomeEnabled: (enabled: boolean) =>
    ipcRenderer.invoke('engine:metronome:setEnabled', enabled),
  setMetronomeVolume: (volume: number) =>
    ipcRenderer.invoke('engine:metronome:setVolume', volume),

  // Phase 2 — State queries
  getTransportState: () => ipcRenderer.invoke('engine:transport:getState'),
  getAudioConfig: () => ipcRenderer.invoke('engine:config:getAudioConfig'),

  // Phase 3 — Command dispatch
  dispatchCommand: (cmd: { command: string; params: Record<string, unknown> }) =>
    ipcRenderer.invoke('command:dispatch', cmd),
  commandUndo: () => ipcRenderer.invoke('command:undo'),
  commandRedo: () => ipcRenderer.invoke('command:redo'),
  getProjectState: () => ipcRenderer.invoke('command:getState'),
  getParameterIds: () => ipcRenderer.invoke('command:getParameterIds'),
  // Phase 4 — Instrument convenience API
  instrumentNoteOn: (instrument: string, note: number, velocity: number) =>
    ipcRenderer.invoke('command:dispatch', {
      command: 'instrument.noteOn',
      params: { instrument, note, velocity }
    }),
  instrumentNoteOff: (instrument: string, note: number) =>
    ipcRenderer.invoke('command:dispatch', {
      command: 'instrument.noteOff',
      params: { instrument, note }
    }),
  drumMachineLoadSample: (padIndex: number, filePath: string) =>
    ipcRenderer.invoke('command:dispatch', {
      command: 'drumMachine.loadSample',
      params: { padIndex, filePath }
    }),

  // Phase 5 — Effect convenience API
  effectInsert: (trackId: string, effectType: string, position?: number) =>
    ipcRenderer.invoke('command:dispatch', {
      command: 'effect.insert',
      params: { trackId, effectType, position: position ?? -1 }
    }),
  effectRemove: (trackId: string, position: number) =>
    ipcRenderer.invoke('command:dispatch', {
      command: 'effect.remove',
      params: { trackId, position }
    }),
  effectReorder: (trackId: string, fromPosition: number, toPosition: number) =>
    ipcRenderer.invoke('command:dispatch', {
      command: 'effect.reorder',
      params: { trackId, fromPosition, toPosition }
    }),
  effectBypass: (trackId: string, position: number, bypassed: boolean) =>
    ipcRenderer.invoke('command:dispatch', {
      command: 'effect.bypass',
      params: { trackId, position, bypassed }
    }),

  // Phase 10.1 — Clip operations
  clipAdd: (clip: {
    clipId: string; trackId: string; startBeat: number; lengthBeats: number;
    notes: Array<{ pitch: number; startBeat: number; lengthBeats: number; velocity: number }>
  }) => ipcRenderer.invoke('engine:clip:add', clip),
  clipRemove: (clipId: string) => ipcRenderer.invoke('engine:clip:remove', clipId),
  clipUpdate: (clip: {
    clipId: string; trackId: string; startBeat: number; lengthBeats: number;
    notes: Array<{ pitch: number; startBeat: number; lengthBeats: number; velocity: number }>
  }) => ipcRenderer.invoke('engine:clip:update', clip),
  clipClear: () => ipcRenderer.invoke('engine:clip:clear'),

  onCommandEvent: (
    callback: (event: { type: string; command: string; data: string }) => void
  ) => ipcRenderer.on('command:event', (_event, data) => callback(data)),
  removeCommandEventListener: () => ipcRenderer.removeAllListeners('command:event'),
})
