// The contract between the renderer and the main process: `window.calliope`.
//
// The preload script implements this interface and the renderer consumes it,
// so a method that exists on one side but not the other is a compile error
// rather than a runtime "No handler registered" message.

export interface TransportState {
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

export interface AudioConfig {
  sampleRate: number
  bufferSize: number
  initialised: boolean
}

export interface MeterLevel {
  rmsLeft: number
  rmsRight: number
  peakLeft: number
  peakRight: number
}

/** Keyed by track ID, plus 'master'. */
export type MeterLevels = Record<string, MeterLevel>

export interface EngineClipPayload {
  clipId: string
  trackId: string
  startBeat: number
  lengthBeats: number
  /** velocity is MIDI 1-127 */
  notes: Array<{ pitch: number; startBeat: number; lengthBeats: number; velocity: number }>
}

export interface EngineCommand {
  command: string
  params: Record<string, unknown>
}

export interface ParameterDescription {
  id: string
  type: string
  min: number | boolean
  max: number | boolean
  value: number | boolean
}

export type ExportFormat = 'wav16' | 'wav24' | 'mp3' | 'flac'
export type ExportFormatSupport = Record<ExportFormat, boolean>

export interface ExportAudioParams {
  outputPath: string
  format: ExportFormat
  mp3Bitrate?: number
  /** Length to render in beats. Omit to render to the end of the last clip. */
  totalBeats?: number
}

export interface ExportStemsParams {
  outputDir: string
  totalBeats?: number
  /** Track ID -> file name (without extension). */
  trackNames: Record<string, string>
}

export interface ProjectSaveRequest {
  contents: string
  /** null or undefined asks the user where to save. */
  filePath?: string | null
  suggestedName?: string
}

export interface ProjectFileResult {
  success: boolean
  canceled?: boolean
  filePath?: string
  /** Present when a project was opened. */
  contents?: string
  error?: string
}

export type MenuAction =
  | 'new'
  | 'open'
  | 'save'
  | 'save-as'
  | 'export'
  | 'undo'
  | 'redo'
  | 'toggle-assistant'
  | 'ai-settings'

/** Removes the listener it was returned for. */
export type Unsubscribe = () => void

export interface CalliopeAPI {
  // Engine information
  getEngineInfo(): Promise<{ juceVersion: string; audioDevices: string[] }>
  startTestTone(frequency: number): Promise<boolean>
  stopTestTone(): Promise<void>

  // Lifecycle
  initialiseEngine(sampleRate: number, bufferSize: number): Promise<boolean>
  shutdownEngine(): Promise<void>
  setBufferSize(bufferSize: number): Promise<boolean>

  // Transport
  transportPlay(): Promise<void>
  transportStop(): Promise<void>
  transportPause(): Promise<void>
  setBpm(bpm: number): Promise<void>
  setTimeSignature(numerator: number, denominator: number): Promise<void>
  setLoopRegion(startBeat: number, endBeat: number, enabled: boolean): Promise<void>
  setMetronomeEnabled(enabled: boolean): Promise<void>
  setMetronomeVolume(volume: number): Promise<void>

  // State queries
  getTransportState(): Promise<TransportState>
  getAudioConfig(): Promise<AudioConfig>
  getMeterLevels(): Promise<MeterLevels>

  // Tracks and clips. The renderer's project/engine-sync is the only caller.
  addTrack(track: { trackId: string; instrument: string }): Promise<boolean>
  removeTrack(trackId: string): Promise<boolean>
  resetProject(): Promise<boolean>
  clipUpdate(clip: EngineClipPayload): Promise<boolean>
  clipRemove(clipId: string): Promise<boolean>

  // Commands and parameters
  dispatchCommand(cmd: EngineCommand): Promise<boolean>
  describeParameters(): Promise<ParameterDescription[]>
  /** Audition a note on a track's instrument. velocity is MIDI 1-127. */
  previewNoteOn(trackId: string, note: number, velocity: number): Promise<boolean>
  previewNoteOff(trackId: string, note: number): Promise<boolean>

  // Project files
  projectSave(request: ProjectSaveRequest): Promise<ProjectFileResult>
  projectOpen(): Promise<ProjectFileResult>
  /** Tells the main process whether closing the window would lose work. */
  projectSetDirty(dirty: boolean): void

  // Export
  getExportFormats(): Promise<ExportFormatSupport>
  showExportPathDialog(format: ExportFormat): Promise<string | null>
  exportAudio(params: ExportAudioParams): Promise<boolean>
  exportStems(params: ExportStemsParams): Promise<boolean>
  onExportProgress(callback: (fraction: number) => void): Unsubscribe

  // Application menu
  onMenuAction(callback: (action: MenuAction) => void): Unsubscribe
}
