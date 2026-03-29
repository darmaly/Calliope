interface TransportState {
  state: 'stopped' | 'playing' | 'paused'
  bpm: number
  timeSigNumerator: number
  timeSigDenominator: number
  samplePosition: number
  ppqPosition: number
  looping: boolean
  loopStartBeat: number
  loopEndBeat: number
}

interface AudioConfig {
  sampleRate: number
  bufferSize: number
  initialised: boolean
}

interface CalliopeAPI {
  // Phase 1
  getEngineInfo(): Promise<{ juceVersion: string; audioDevices: string[] }>
  startTestTone(frequency: number): Promise<boolean>
  stopTestTone(): Promise<void>

  // Phase 2 — Engine lifecycle
  initialiseEngine(sampleRate: number, bufferSize: number): Promise<boolean>
  shutdownEngine(): Promise<void>

  // Phase 2 — Transport
  transportPlay(): Promise<void>
  transportStop(): Promise<void>
  transportPause(): Promise<void>
  setBpm(bpm: number): Promise<void>
  setTimeSignature(num: number, den: number): Promise<void>
  setLoopRegion(startBeat: number, endBeat: number, enabled: boolean): Promise<void>

  // Phase 2 — Config
  setBufferSize(bufferSize: number): Promise<boolean>

  // Phase 2 — Metronome
  setMetronomeEnabled(enabled: boolean): Promise<void>
  setMetronomeVolume(volume: number): Promise<void>

  // Phase 2 — State queries
  getTransportState(): Promise<TransportState>
  getAudioConfig(): Promise<AudioConfig>

  // Phase 3 — Command dispatch
  dispatchCommand(cmd: { command: string; params: Record<string, unknown> }): Promise<unknown>
  commandUndo(): Promise<void>
  commandRedo(): Promise<void>
  getProjectState(): Promise<unknown>
  getParameterIds(): Promise<string[]>

  // Phase 4 — Instrument convenience API
  instrumentNoteOn(instrument: string, note: number, velocity: number): Promise<unknown>
  instrumentNoteOff(instrument: string, note: number): Promise<unknown>
  drumMachineLoadSample(padIndex: number, filePath: string): Promise<unknown>

  // Phase 5 — Effect convenience API
  effectInsert(trackId: string, effectType: string, position?: number): Promise<unknown>
  effectRemove(trackId: string, position: number): Promise<unknown>
  effectReorder(trackId: string, fromPosition: number, toPosition: number): Promise<unknown>
  effectBypass(trackId: string, position: number, bypassed: boolean): Promise<unknown>

  // Phase 8 — Mixer
  getMeterLevels(): Promise<Record<string, { rmsLeft: number; rmsRight: number; peakLeft: number; peakRight: number }>>
  setTrackVolume(trackId: string, volume: number): Promise<unknown>
  setTrackPan(trackId: string, pan: number): Promise<unknown>

  // Event subscription
  onCommandEvent(callback: (event: { type: string; command: string; data: string }) => void): void
  removeCommandEventListener(): void

  // Phase 9 — Project save/load
  projectSave(filePath?: string): Promise<{ success: boolean; filePath: string | null }>
  projectSaveAs(): Promise<{ success: boolean; filePath: string | null }>
  projectLoad(): Promise<{ success: boolean; filePath: string | null }>
  projectNew(): Promise<{ success: boolean }>
  projectGetInfo(): Promise<{ filePath: string | null; isDirty: boolean }>
  projectSetAutosave(enabled: boolean, intervalMs?: number): Promise<{ autosaveEnabled: boolean; autosaveIntervalMs: number }>
  projectGetAutosaveConfig(): Promise<{ autosaveEnabled: boolean; autosaveIntervalMs: number }>
  projectMarkDirty(): Promise<void>
  onProjectAutosaved(callback: (data: { filePath: string; timestamp: string }) => void): void
  removeProjectAutosavedListener(): void

  // Phase 9 — Export
  exportAudio(params: {
    outputPath: string
    format: string
    mp3Bitrate: number
    totalBeats: number
    midiEventsJson: string
  }): Promise<boolean>
  exportStems(params: {
    outputDir: string
    totalBeats: number
    midiEventsJson: string
  }): Promise<boolean>
  loadProjectState(json: string): Promise<boolean>
  onExportProgress(callback: (percent: number) => void): void
  removeExportProgressListener(): void
  showExportPathDialog(format: string): Promise<string | null>
  onShowExportDialog(callback: () => void): void
  removeShowExportDialogListener(): void
}

declare global {
  interface Window {
    calliope: CalliopeAPI
  }
}

export {}
