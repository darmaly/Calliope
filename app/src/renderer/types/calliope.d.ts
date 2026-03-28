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
}

declare global {
  interface Window {
    calliope: CalliopeAPI
  }
}

export {}
