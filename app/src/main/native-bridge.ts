import { createRequire } from 'module'
import { app } from 'electron'
import path from 'path'

const require = createRequire(import.meta.url)

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

interface NativeAddon {
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
  setTimeSignature(numerator: number, denominator: number): Promise<void>
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
  // Phase 4 — Instrument command types (dispatched via dispatchCommand):
  // { command: 'instrument.noteOn', params: { instrument: 'polysynth'|'basssynth'|'drumMachine', note: number, velocity: number } }
  // { command: 'instrument.noteOff', params: { instrument: 'polysynth'|'basssynth'|'drumMachine', note: number } }
  // { command: 'drumMachine.loadSample', params: { padIndex: number, filePath: string } }
  dispatchCommand(cmd: { command: string; params: Record<string, unknown> }): Promise<boolean>
  commandUndo(): Promise<boolean>
  commandRedo(): Promise<boolean>
  getProjectState(): Promise<string>
  getParameterIds(): Promise<string[]>
  subscribeToEvents(
    callback: (event: { type: string; command: string; data: string }) => void
  ): void
  unsubscribeFromEvents(): void
}

function loadAddon(): NativeAddon {
  // In dev: addon is at project_root/build/Release/calliope_addon.node
  // In production: addon is unpacked from asar
  const addonPath = app.isPackaged
    ? path.join(process.resourcesPath, 'build', 'Release', 'calliope_addon.node')
    : path.join(__dirname, '..', '..', '..', 'build', 'Release', 'calliope_addon.node')
  return require(addonPath) as NativeAddon
}

export const native = loadAddon()
