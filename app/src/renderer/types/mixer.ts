export interface EffectSlotInfo {
  effectType: string
  bypassed: boolean
}

export const EFFECT_TYPES = [
  { type: 'eq', label: 'Parametric EQ' },
  { type: 'compressor', label: 'Compressor' },
  { type: 'reverb', label: 'Reverb' },
  { type: 'delay', label: 'Delay' },
  { type: 'limiter', label: 'Limiter' },
] as const

export type EffectType = (typeof EFFECT_TYPES)[number]['type']

export interface LevelData {
  peakL: number
  peakR: number
}

export interface MixerState {
  // Visibility
  mixerVisible: boolean
  // Per-track mixer data (keyed by track id)
  trackVolumes: Record<string, number>       // 0.0 to ~1.259 (+6dB), default 1.0
  trackPans: Record<string, number>          // -1.0 to 1.0, default 0.0
  trackLevels: Record<string, LevelData>     // real-time meter data
  trackEffects: Record<string, EffectSlotInfo[]>  // per-track effect chain
  // Master
  masterVolume: number   // 0.0 to ~1.259, default 1.0
  masterPan: number      // -1.0 to 1.0, default 0.0
  masterLevel: LevelData // real-time meter data
  masterEffects: EffectSlotInfo[]
  // Panel height
  mixerHeight: number   // default 300, min 240
}

export interface MixerActions {
  // Visibility
  toggleMixerVisible: () => void
  setMixerVisible: (visible: boolean) => void
  // Per-track volume/pan
  setTrackVolume: (trackId: string, volume: number) => void
  setTrackPan: (trackId: string, pan: number) => void
  getTrackVolume: (trackId: string) => number
  getTrackPan: (trackId: string) => number
  // Level meters
  setTrackLevel: (trackId: string, level: LevelData) => void
  setMasterLevel: (level: LevelData) => void
  // Master
  setMasterVolume: (volume: number) => void
  setMasterPan: (pan: number) => void
  // Effects
  getTrackEffects: (trackId: string) => EffectSlotInfo[]
  addTrackEffect: (trackId: string, effectType: string) => void
  removeTrackEffect: (trackId: string, index: number) => void
  reorderTrackEffect: (trackId: string, fromIndex: number, toIndex: number) => void
  bypassTrackEffect: (trackId: string, index: number, bypassed: boolean) => void
  // Master effects
  addMasterEffect: (effectType: string) => void
  removeMasterEffect: (index: number) => void
  reorderMasterEffect: (fromIndex: number, toIndex: number) => void
  bypassMasterEffect: (index: number, bypassed: boolean) => void
  // Panel height
  setMixerHeight: (height: number) => void
  // Cleanup
  removeTrackData: (trackId: string) => void
}
