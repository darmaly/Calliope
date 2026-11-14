import { createRequire } from 'module'
import { app } from 'electron'
import path from 'path'
import type {
  AudioConfig,
  EngineClipPayload,
  EngineCommand,
  ExportAudioParams,
  ExportFormatSupport,
  ExportStemsParams,
  MeterLevels,
  ParameterDescription,
  TransportState,
} from '../shared/api'

const require = createRequire(import.meta.url)

/**
 * The C++ addon (native/src/bridge.cpp). Every method returns a Promise, but
 * all except device open/close and export do their work synchronously inside
 * the call, so calls take effect in the order they are made.
 */
export interface NativeAddon {
  getEngineInfo(): Promise<{ juceVersion: string; audioDevices: string[] }>
  startTestTone(frequency: number): Promise<boolean>
  stopTestTone(): Promise<void>

  initialiseEngine(sampleRate: number, bufferSize: number): Promise<boolean>
  shutdownEngine(): Promise<void>
  setBufferSize(bufferSize: number): Promise<boolean>

  transportPlay(): Promise<void>
  transportStop(): Promise<void>
  transportPause(): Promise<void>
  setBpm(bpm: number): Promise<void>
  setTimeSignature(numerator: number, denominator: number): Promise<void>
  setLoopRegion(startBeat: number, endBeat: number, enabled: boolean): Promise<void>
  setMetronomeEnabled(enabled: boolean): Promise<void>
  setMetronomeVolume(volume: number): Promise<void>

  getTransportState(): Promise<TransportState>
  getAudioConfig(): Promise<AudioConfig>
  getMeterLevels(): Promise<MeterLevels>

  addTrack(track: { trackId: string; instrument: string }): Promise<boolean>
  removeTrack(trackId: string): Promise<boolean>
  resetProject(): Promise<boolean>

  addClip(clip: EngineClipPayload): Promise<boolean>
  updateClip(clip: EngineClipPayload): Promise<boolean>
  removeClip(clipId: string): Promise<boolean>
  clearClips(): Promise<boolean>

  dispatchCommand(cmd: EngineCommand): Promise<boolean>
  commandUndo(): Promise<boolean>
  commandRedo(): Promise<boolean>
  getProjectState(): Promise<string>
  loadProjectState(json: string): Promise<boolean>
  getParameterIds(): Promise<string[]>
  describeParameters(): Promise<ParameterDescription[]>
  subscribeToEvents(callback: (event: { type: string; command: string; data: string }) => void): void
  unsubscribeFromEvents(): void

  getExportFormats(): ExportFormatSupport
  exportAudio(params: ExportAudioParams, onProgress?: (fraction: number) => void): Promise<boolean>
  exportStems(params: ExportStemsParams, onProgress?: (fraction: number) => void): Promise<boolean>
}

function loadAddon(): NativeAddon {
  // In dev: addon is at project_root/build/Release/calliope_addon.node
  // In production: addon is unpacked from asar
  const addonPath = app.isPackaged
    ? path.join(process.resourcesPath, 'app.asar.unpacked', 'build', 'Release', 'calliope_addon.node')
    : path.join(__dirname, '..', '..', '..', 'build', 'Release', 'calliope_addon.node')
  return require(addonPath) as NativeAddon
}

export const native = loadAddon()
