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

interface MeterLevels {
  [trackId: string]: { peakLeft: number; peakRight: number }
}

interface ProjectResult {
  success: boolean
  filePath?: string
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

  // Phase 10.1 — Clip operations
  clipAdd(clip: {
    clipId: string; trackId: string; startBeat: number; lengthBeats: number;
    notes: Array<{ pitch: number; startBeat: number; lengthBeats: number; velocity: number }>
  }): Promise<boolean>
  clipRemove(clipId: string): Promise<boolean>
  clipUpdate(clip: {
    clipId: string; trackId: string; startBeat: number; lengthBeats: number;
    notes: Array<{ pitch: number; startBeat: number; lengthBeats: number; velocity: number }>
  }): Promise<boolean>
  clipClear(): Promise<boolean>

  // Event subscription
  onCommandEvent(callback: (event: { type: string; command: string; data: string }) => void): void
  removeCommandEventListener(): void

  // Phase 8 — Mixer / Metering
  getMeterLevels(): Promise<MeterLevels>
  setTrackVolume(trackId: string, volume: number): Promise<unknown>
  setTrackPan(trackId: string, pan: number): Promise<unknown>

  // Phase 9 — Project save/load
  projectSave(filePath?: string): Promise<ProjectResult>
  projectSaveAs(): Promise<ProjectResult>
  projectLoad(): Promise<ProjectResult>
  projectNew(): Promise<void>
  projectGetInfo(): Promise<unknown>
  projectSetAutosave(enabled: boolean, intervalMs?: number): Promise<void>
  projectGetAutosaveConfig(): Promise<unknown>
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
  }): Promise<unknown>
  exportStems(params: {
    outputDir: string
    totalBeats: number
    midiEventsJson: string
  }): Promise<unknown>
  loadProjectState(json: string): Promise<unknown>
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
