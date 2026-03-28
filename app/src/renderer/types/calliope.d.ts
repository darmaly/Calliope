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

  // Event subscription
  onCommandEvent(callback: (event: { type: string; command: string; data: string }) => void): void
  removeCommandEventListener(): void
}

declare global {
  interface Window {
    calliope: CalliopeAPI
  }
}

export {}
